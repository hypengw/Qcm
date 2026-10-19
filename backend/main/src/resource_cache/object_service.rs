use super::super::store::{Object, ObjectMetadata, MAX_OBJECT};
use super::*;

impl ResourceCache {
    pub async fn object_response(
        self: &Arc<Self>,
        request: ResourceRequest,
    ) -> Result<(Response<ResponseBody>, reqwest::Url), ProcessError> {
        let (response, url) = self
            .respond_object(request)
            .await
            .map_err(ProcessError::Resource)?;
        Ok((
            response.into_response().map_err(ProcessError::Resource)?,
            url,
        ))
    }
    pub(super) async fn respond_object(
        self: &Arc<Self>,
        request: ResourceRequest,
    ) -> Result<(ResourceDelivery, reqwest::Url)> {
        ensure!(
            !*self.stop.borrow() && request.prepared.is_valid(),
            "playback resource unavailable"
        );
        let permit = self.readers.clone().acquire_owned().await?;
        let buffer = self.buffers.clone().acquire_owned().await?;
        let prepared = request.prepared.clone();
        let base = request_headers(prepared.headers(), &request.headers);
        let key = identity(&serde_json::to_vec(&(
            request.provider,
            &request.scope,
            prepared.cache_url(prepared.url()).as_str(),
            canonical_headers(&base),
        ))?)?;
        let open = {
            let mut opens = self.opens.lock().await;
            opens.retain(|_, v| v.strong_count() > 0);
            if let Some(lock) = opens.get(&key).and_then(Weak::upgrade) {
                lock
            } else {
                let lock = Arc::new(Mutex::new(()));
                opens.insert(key.clone(), Arc::downgrade(&lock));
                lock
            }
        };
        let _opened = open.lock().await;
        let bypass = policy::request_no_store(&request.headers);
        let cached = if !bypass {
            if let Some(store) = &self.store {
                store.object(key.clone()).await.unwrap_or_else(|e| {
                    log::warn!("object cache lookup failed: {e}");
                    None
                })
            } else {
                None
            }
        } else {
            None
        };
        let fresh = cached
            .as_ref()
            .is_some_and(|o| o.meta.policy.fresh(&request.headers));
        if policy::only_cached(&request.headers) && !fresh {
            return Ok((
                empty(StatusCode::GATEWAY_TIMEOUT, HeaderMap::new())?,
                prepared.url().clone(),
            ));
        }
        let object = if fresh {
            cached.unwrap()
        } else {
            let mut headers = base.clone();
            if let Some(old) = &cached {
                headers.extend(old.meta.policy.conditional());
            }
            let sent = policy::now();
            let transfer = self.requests.clone().acquire_owned().await?;
            let mut response = self.execute(&prepared, headers).await?;
            let effective = response.url().clone();
            if response.status() == StatusCode::NOT_MODIFIED {
                let mut old = cached.ok_or_else(|| anyhow::anyhow!("unexpected object 304"))?;
                ensure!(
                    identity(prepared.cache_url(&effective).as_str().as_bytes())?
                        == old.meta.target,
                    "object validation target changed"
                );
                for (name, expected) in [
                    ("etag", old.meta.policy.etag.as_deref()),
                    ("last-modified", old.meta.policy.modified.as_deref()),
                ] {
                    if let Some(value) = policy::value(response.headers(), name) {
                        ensure!(Some(value) == expected, "object validator changed");
                    }
                }
                if let Some(length) = policy::value(response.headers(), "content-length") {
                    ensure!(
                        length.parse::<u64>().ok() == Some(old.meta.length),
                        "object validation length changed"
                    );
                }
                let merged = policy::merge_validation(&old.meta.headers, response.headers());
                old.meta.policy = Policy::from_response(&merged, sent, policy::now());
                old.meta.headers = policy::saved(&merged);
                if let Some(store) = &self.store {
                    if old.meta.policy.store {
                        let _ = store.update_object(old.meta.clone()).await;
                    } else {
                        let _ = store.remove_object(key.clone()).await;
                    }
                }
                old
            } else {
                drop(cached);
                if response.status() != StatusCode::OK
                    || response.content_length().is_some_and(|n| n > MAX_OBJECT)
                {
                    if response.status() == StatusCode::OK && request.headers.contains_key("range")
                    {
                        drop(response);
                        response = self
                            .execute(&prepared, downstream_request(&base, &request.headers))
                            .await?;
                    }
                    let effective = response.url().clone();
                    return Ok((
                        streaming_object(
                            response,
                            vec![],
                            permit,
                            buffer,
                            transfer,
                            prepared,
                            self.stop.subscribe(),
                        ),
                        effective,
                    ));
                }
                let policy = Policy::from_response(response.headers(), sent, policy::now());
                let saved = policy::saved(response.headers());
                let expected = response.content_length();
                let mut data = BytesMut::new();
                while let Some(chunk) = tokio::select! {
                    chunk=tokio::time::timeout(Duration::from_secs(30),response.chunk())=>chunk??,
                    _=prepared.cancelled()=>anyhow::bail!("playback resource invalidated"),
                    _=self.wait_stopped()=>anyhow::bail!("resource service stopped"),
                } {
                    ensure!(
                        prepared.is_valid() && !*self.stop.borrow(),
                        "playback resource invalidated"
                    );
                    if data.len() as u64 + chunk.len() as u64 > MAX_OBJECT {
                        if request.headers.contains_key("range") {
                            drop(response);
                            drop(data);
                            let response = self
                                .execute(&prepared, downstream_request(&base, &request.headers))
                                .await?;
                            let effective = response.url().clone();
                            return Ok((
                                streaming_object(
                                    response,
                                    vec![],
                                    permit,
                                    buffer,
                                    transfer,
                                    prepared,
                                    self.stop.subscribe(),
                                ),
                                effective,
                            ));
                        }
                        return Ok((
                            streaming_object(
                                response,
                                vec![data.freeze(), chunk],
                                permit,
                                buffer,
                                transfer,
                                prepared,
                                self.stop.subscribe(),
                            ),
                            effective,
                        ));
                    }
                    data.extend_from_slice(&chunk);
                }
                ensure!(
                    expected.is_none_or(|n| n == data.len() as u64),
                    "incomplete object response"
                );
                ensure!(
                    prepared.is_valid() && !*self.stop.borrow(),
                    "playback resource invalidated"
                );
                let meta = ObjectMetadata {
                    key: key.clone(),
                    group: identity(request.scope.as_bytes())?,
                    provider: request.provider,
                    length: data.len() as u64,
                    headers: saved,
                    policy,
                    target: identity(prepared.cache_url(&effective).as_str().as_bytes())?,
                    effective_url: prepared.cache_url(&effective).to_string(),
                };
                let object = Object {
                    meta,
                    bytes: data.freeze(),
                };
                if !bypass && object.meta.policy.store {
                    if let Some(store) = &self.store {
                        let guard = prepared.clone();
                        if let Err(err) = store
                            .put_object(object.meta.clone(), object.bytes.clone(), move || {
                                guard.is_valid()
                            })
                            .await
                        {
                            log::warn!("object cache write failed: {err}");
                        }
                        if !prepared.is_valid() {
                            let _ = store.remove_object(key).await;
                        }
                    }
                } else if let Some(store) = &self.store {
                    let _ = store.remove_object(key).await;
                }
                object
            }
        };
        ensure!(prepared.is_valid(), "playback invalidated");
        let effective = reqwest::Url::parse(&object.meta.effective_url)?;
        let mut headers = policy::restore(&object.meta.headers);
        if policy::not_modified(&object.meta.policy, &request.headers) {
            return Ok((empty(StatusCode::NOT_MODIFIED, headers)?, effective));
        }
        let meta = Metadata {
            key: object.meta.key,
            provider: request.provider,
            kind: request.kind,
            generation: uuid::Uuid::new_v4().simple().to_string(),
            geometry: Geometry::new(object.meta.length)?,
            headers: object.meta.headers,
            policy: object.meta.policy,
            target: object.meta.target,
            ranges: true,
            layout: Default::default(),
        };
        let range = client_range(&request.headers, &meta)?;
        let Some(range) = range else {
            headers.insert(
                "content-range",
                format!("bytes */{}", meta.geometry.length).parse()?,
            );
            return Ok((
                empty(StatusCode::RANGE_NOT_SATISFIABLE, headers)?,
                effective,
            ));
        };
        let partial = range_requested(&request.headers, &meta);
        headers.insert(
            "content-length",
            (range.end - range.start).to_string().parse()?,
        );
        headers.insert("accept-ranges", "bytes".parse()?);
        headers.insert("age", meta.policy.age().to_string().parse()?);
        if partial {
            headers.insert(
                "content-range",
                format!(
                    "bytes {}-{}/{}",
                    range.start,
                    range.end - 1,
                    meta.geometry.length
                )
                .parse()?,
            );
        }
        let body = futures_util::stream::unfold(
            (object.bytes, range, permit, buffer, prepared),
            |(bytes, mut range, permit, buffer, prepared)| async move {
                if range.start == range.end {
                    return None;
                }
                if !prepared.is_valid() {
                    range.start = range.end;
                    return Some((
                        Err(HttpError::Cache("playback invalidated".into())),
                        (bytes, range, permit, buffer, prepared),
                    ));
                }
                let end = range.end.min(range.start + OUTPUT_BYTES as u64);
                let chunk = bytes.slice(range.start as usize..end as usize);
                range.start = end;
                Some((
                    Ok(Frame::data(chunk)),
                    (bytes, range, permit, buffer, prepared),
                ))
            },
        );
        let mut response =
            Response::new(ResponseBody::Boxed(BodyExt::boxed(StreamBody::new(body))));
        *response.status_mut() = if partial {
            StatusCode::PARTIAL_CONTENT
        } else {
            StatusCode::OK
        };
        *response.headers_mut() = headers;
        Ok((ResourceDelivery::Body(response), effective))
    }
}

fn streaming_object(
    response: reqwest::Response,
    prefix: Vec<Bytes>,
    reader: OwnedSemaphorePermit,
    buffer: OwnedSemaphorePermit,
    transfer: OwnedSemaphorePermit,
    prepared: Arc<PreparedResource>,
    stop: watch::Receiver<bool>,
) -> ResourceDelivery {
    let status = response.status();
    let headers = policy::forwarded(response.headers());
    let stream = futures_util::stream::iter(prefix.into_iter().map(|bytes| Ok(Frame::data(bytes))))
        .chain(
            response
                .bytes_stream()
                .map_ok(Frame::data)
                .map_err(HttpError::from),
        );
    let body = futures_util::stream::unfold(
        (
            Some(Box::pin(stream)),
            reader,
            buffer,
            transfer,
            prepared,
            stop,
        ),
        |(stream, reader, buffer, transfer, prepared, mut stop)| async move {
            let mut stream = stream?;
            let item = if !prepared.is_valid() || *stop.borrow() {
                Some(Err(HttpError::Cache("playback closed".into())))
            } else {
                tokio::select! {
                    item=stream.next()=>item,
                    _=prepared.cancelled()=>Some(Err(HttpError::Cache("playback closed".into()))),
                    _=stop.changed()=>Some(Err(HttpError::Cache("resource service stopped".into()))),
                }
            };
            let item = item?;
            let next = if item.is_ok() { Some(stream) } else { None };
            Some((item, (next, reader, buffer, transfer, prepared, stop)))
        },
    );
    let mut response = Response::new(ResponseBody::Boxed(BodyExt::boxed(StreamBody::new(body))));
    *response.status_mut() = status;
    *response.headers_mut() = headers;
    ResourceDelivery::Body(response)
}

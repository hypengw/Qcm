
## <a id="获取上传地址"></a>获取上传地址

### <a id="接口描述"></a>接口描述

域名：http://wanproxy.127.net

接口名：/lbs?version=1.0&bucketname={bucket}

用于获取上传加速节点地址。

### <a id="输入参数"></a>输入参数

| 参数  | 类型  | 必须  | 说明  |
| :--- | :--- | :--- | :--- |
| version | String | 是   | API版本号，填写固定值1.0 |
| bucketname | String | 是   | 存储上传文件的桶名，可在视频上传初始化接口的返回参数bucket获取 |

### <a id="输出参数"></a>输出参数

| 参数  | 类型  | 说明  |
| :--- | :--- | :--- |
| lbs | String | httpDNS的IP访问地址，用于避免后续查询的DNS解析时间、以及域名劫持 |
| upload | List | 上传节点列表（前面的优先级高） |

### <a id="响应状态码"></a>响应状态码

| 参数  | 类型  | 说明  |
| :--- | :--- | :--- |
| Code | String | 错误代码 |
| Message | String | 错误描述信息 |

### <a id="示例代码"></a>示例代码

**输入1**

```
curl -X GET http://wanproxy.127.net/lbs?version=1.0&bucketname=vod**ywxdf
```

```

```

**输出1**

```
	{"lbs":"http://223.252.**.**/lbs","upload"：["http://223.252.**.**","http://223.252.**.**"]}
```

```

```

## <a id="媒资上传"></a>媒资上传

上传一块数据，此接口通过指定offset实现断点续传功能。用户每次上传要以服务器端返回的offset为准续传余下数据。

- 支持通过服务端接口或媒体上传 SDK 进行媒资上传。推荐使用媒体上传 SDK 进行媒资上传，具体请参见[媒体上传 SDK](https://doc.yunxin.163.com/docs/jY3NDM4Nzc/jM1NTk4NTA?platformId=100002)。
- 如果上传大文件，请使用分片上传，分片大小不超过 4 MB。请求相同接口，注意参数的填写。
- 如果需要断点续传，需保存 context 值，根据 context 值查询断点值，然后继续使用此接口。
- 包体数据为二进制数据。

### <a id="接口描述"></a>接口描述

POST {UploadHost}/{bucket}/{object}

- {UploadHost}值为获取的上传加速节点地址。
- {bucket}值为存储对象的桶名。
- {object}值为生成的唯一对象名。

### <a id="输入参数"></a>输入参数

| 参数  | 类型  | 必须  | 说明  |
| :--- | :--- | :--- | :--- |
| x-nos-token | String | 是   | 请求头参数，上传token |
| Content-Length | long | 否   | 请求头参数，当前片的内容长度，  <br>单位：字节（Byte）。Content-Length合法值是\[0~4M\]，  <br>否则返回400 httpcode给客户端，拒绝本次请求 |
| Content-Type | String | 否   | 请求头参数，标准http头。表示请求内容的类型，  <br>比如：image/jpeg。 仅第一次上传生效，续传不生效 |
| Content-MD5 | String | 否   | 请求头参数，文件内容md5值 |
| bucket | String | 是   | 存储对象的桶名 |
| object | String | 是   | 生成的唯一对象名 |
| offset | long | 是   | 当前分片在整个对象中的起始偏移量，单位：字节（Byte） |
| complete | String | 是   | 是否为最后一块数据。合法值：true/false |
| version | String | 是   | http api版本号。这里是固定值1.0 |
| context | String | 是   | 上传上下文。本字段是只能被上传服务器解读使用的不透明字段，  <br>上传端不应修改其内容。  <br>注意：用户第一次上传应不带此参数或置为空字符串，  <br>之后上传剩余部分数据都需要带上这个参数。  <br>context对应的桶名或者对象名不匹配返回400 code |

### <a id="输出参数"></a>输出参数

### 响应成功输出参数

| 参数  | 类型  | 必须  | 说明  |
| :--- | :--- | :--- | :--- |
| requestId | String | 是   | uuid字符串，服务器端生成的唯一UUID |
| offset | long | 是   | 下一个上传片在上传块中的偏移。  <br>注意：偏移从0开始，比如：用户上传0-128字节后，  <br>服务器返回的offset为128，下一次上传offset值应置为128 |
| context | String | 是   | 上传上下文 |
| callbackRetMsg | String | 是   | 上传回调信息 |

### 响应失败输出参数

| 参数  | 类型  | 说明  |
| :--- | :--- | :--- |
| requestId | String | uuid字符串，服务器端生成的唯一UUID |
| errMsg | String | 错误描述信息 |

### <a id="响应状态码"></a>响应状态码

| 状态码 | 含义  |
| :--- | :--- |
| 200 | 上传分片成功 |
| 400 | 请求报文格式错误，报文构造不正确或者没有完整发送 |
| 403 | 上传凭证无效。token过期服务器会返回此状态码，用户需要重新申请token |
| 500 | 服务器内部出现错误，请稍后重试或者将完整错误信息发送给客服人员帮忙解决 |
| 520 | 回调失败 |

### <a id="示例代码"></a>示例代码

**输入1**

```
		curl -X POST -H "Content-Length: 4194304" -H "x-nos-token: UPLOAD ab1****e5ee:n5VKrOLVFkLM7JI***2OTk1NTc5NjU4In0=" -d'[本次上传视频文件二进制内容数据]' "http://223.252.**.**/vodk32ywxdf/d37906a7-0119-***c66a71952ad.mp4?offset=0&complete=false&version=1.0"
```

```

```

**输出1**

```
	{
		"requestId":"be82c2a0****189d831",
		"offset":4194304,
		"context":"f3e2681*****9ff47af21c7",
		"callbackRetMsg":""
	}
```

```

```

## <a id="查询长传断点"></a>查询长传断点

根据上传上下文查询对应分片上传当前续传的offset，上下文要与bucketName/objectName匹配，否则返回400状态码。(bucketName和objectName要进行URL编码,字符编码格式使用utf-8)。

### <a id="接口描述"></a>接口描述

GET {UploadHost}/{bucket}/{object}?uploadContext

- {UploadHost}值为获取的上传加速节点地址。
- {bucket}值为存储对象的桶名。
- {object}值为生成的唯一对象名。

### <a id="输入参数"></a>输入参数

| 参数  | 类型  | 必须  | 说明  |
| :--- | :--- | :--- | :--- |
| x-nos-token | String | 是   | 上传token |
| bucketName | String | 是   | 存储对象的桶名 |
| objectName | String | 是   | 生成的唯一对象名 |
| context | String | 是   | 上传上下文。本字段是只能被上传服务器解读使用的不透明字段，  <br>上传端不应修改其内容。对应context在服务端不存在则返回404。  <br>context对应的桶名或者对象名不匹配返回400 code。 |
| version | String | 是   | http api版本号。这里是固定值1.0 |

### <a id="输出参数"></a>输出参数

### 响应成功输出参数

| 参数  | 类型  | 必须  | 说明  |
| :--- | :--- | :--- | :--- |
| requestId | String | 是   | uuid字符串，服务器端生成的唯一UUID，用于记录日志排查问题使用 |
| offset | long | 是   | 下一个上传片在上传块中的偏移 |

### 响应失败输出参数

| 参数  | 类型  | 说明  |
| :--- | :--- | :--- |
| requestId | String | uuid字符串，服务器端生成的唯一UUID |
| errMsg | String | 错误描述信息 |

### <a id="响应状态码"></a>响应状态码

| 状态码 | 含义  |
| :--- | :--- |
| 200 | 上传分片成功 |
| 400 | 请求报文格式错误，报文构造不正确或者没有完整发送 |
| 403 | 上传凭证无效。token过期服务器会返回此错误码，用户需要重新申请token |
| 500 | 服务器内部出现错误，请稍后重试或者将完整错误信息发送给客服人员帮忙解决 |
| 404 | 对应context上传不存在 |

### <a id="示例代码"></a>示例代码

**输入1**

```
curl -X GET -H "x-nos-token: UPLOAD ab1856bb****4e1b8e5ee:n5VKrOLVFkL****DY2OTk1NTc5NjU4In0=" "http://223.252.**.**/vodk32ywxdf/d37906a7-0119-****71952ad.mp4?uploadContext&context=f3e26818-83****1c7&version=1.0"
```

```

```

**输出1**

```
{
	"requestId":"be82c2a****70a189d832",
	"offset":4194304
}
```
pub mod basic;
pub mod fts;
pub mod size;
pub mod sync;
pub mod values;

pub use basic::QueryBuilder;
pub use const_chunks::IteratorConstChunks;
use sea_orm::{
    sea_query::{self, IntoIden, OnConflict},
    ActiveModelTrait, DatabaseTransaction, EntityTrait, InsertResult, PrimaryKeyTrait,
    TryInsertResult,
};
use strum::IntoEnumIterator;

pub fn columns_contains<Col>(list: &[Col], c: &Col) -> bool {
    use std::mem::discriminant;
    list.iter().any(|l| discriminant(l) == discriminant(c))
}

type EntityKey<Et> =
    <<Et as EntityTrait>::PrimaryKey as PrimaryKeyTrait>::ValueType;

pub struct DbOper {}
impl DbOper {
    pub async fn insert_on<Et, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: OnConflict,
    ) -> Result<TryInsertResult<InsertResult<A>>, sea_orm::DbErr>
    where
        Et: EntityTrait,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        Et::insert_many(iter)
            .on_conflict(conflict)
            .on_empty_do_nothing()
            .exec(txn)
            .await
    }

    pub async fn insert<Et, Col, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: &[Col],
        exclude: &[Col],
    ) -> Result<TryInsertResult<InsertResult<A>>, sea_orm::DbErr>
    where
        Et: EntityTrait,
        Col: IntoIden + Copy + IntoEnumIterator,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        Self::insert_on::<Et, A, I>(
            txn,
            iter,
            sea_query::OnConflict::columns(conflict.iter().copied())
                .update_columns(
                    Col::iter()
                        .filter(|e| {
                            !columns_contains(&conflict, e) && !columns_contains(&exclude, e)
                        })
                        .collect::<Vec<_>>(),
                )
                .to_owned(),
        )
        .await
    }
    pub async fn insert_on_return_key<Et, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: OnConflict,
    ) -> Result<TryInsertResult<Vec<EntityKey<Et>>>, sea_orm::DbErr>
    where
        Et: EntityTrait,
        Et::Model: sea_orm::ModelTrait + sea_orm::IntoActiveModel<A>,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        Et::insert_many(iter)
            .on_conflict(conflict)
            .on_empty_do_nothing()
            .exec_with_returning_keys(txn)
            .await
    }

    pub async fn insert_return_key<Et, Col, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: &[Col],
        exclude: &[Col],
    ) -> Result<
        TryInsertResult<Vec<EntityKey<Et>>>,
        sea_orm::DbErr,
    >
    where
        Et: EntityTrait,
        Et::Model: sea_orm::ModelTrait + sea_orm::IntoActiveModel<A>,
        Col: IntoIden + Copy + IntoEnumIterator,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        Self::insert_on_return_key::<Et, A, I>(
            txn,
            iter,
            sea_query::OnConflict::columns(conflict.iter().copied())
                .update_columns(
                    Col::iter()
                        .filter(|e| {
                            !columns_contains(&conflict, e) && !columns_contains(&exclude, e)
                        })
                        .collect::<Vec<_>>(),
                )
                .to_owned(),
        )
        .await
    }
}

fn extend_inserted_keys<PK>(
    out: &mut Vec<PK>,
    res: TryInsertResult<Vec<PK>>,
) -> Result<(), sea_orm::DbErr> {
    match res {
        TryInsertResult::Inserted(inserted) => {
            out.extend(inserted);
            Ok(())
        }
        _ => Err(sea_orm::DbErr::RecordNotInserted),
    }
}

pub struct DbChunkOper<const N: usize> {}

impl<const N: usize> DbChunkOper<N> {
    pub async fn insert_on<Et, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: OnConflict,
    ) -> Result<(), sea_orm::DbErr>
    where
        Et: EntityTrait,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        let mut chunks = iter.into_iter().const_chunks::<N>();
        for iter in &mut chunks {
            DbOper::insert_on(&txn, iter, conflict.clone()).await?;
        }

        if let Some(rm) = chunks.into_remainder() {
            DbOper::insert_on(&txn, rm.into_iter(), conflict).await?;
        }
        Ok(())
    }

    pub async fn insert<Et, Col, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: &[Col],
        exclude: &[Col],
    ) -> Result<(), sea_orm::DbErr>
    where
        Et: EntityTrait,
        Col: IntoIden + Copy + IntoEnumIterator,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        let mut chunks = iter.into_iter().const_chunks::<N>();
        for iter in &mut chunks {
            DbOper::insert(&txn, iter, &conflict, &exclude).await?;
        }

        if let Some(rm) = chunks.into_remainder() {
            DbOper::insert(&txn, rm.into_iter(), &conflict, &exclude).await?;
        }
        Ok(())
    }

    pub async fn insert_return_key<Et, Col, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: &[Col],
        exclude: &[Col],
    ) -> Result<Vec<EntityKey<Et>>, sea_orm::DbErr>
    where
        Et: EntityTrait,
        Et::Model: sea_orm::ModelTrait + sea_orm::IntoActiveModel<A>,
        Col: IntoIden + Copy + IntoEnumIterator,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        let mut out = Vec::new();
        let mut chunks = iter.into_iter().const_chunks::<N>();
        for iter in &mut chunks {
            let res = DbOper::insert_return_key(&txn, iter, &conflict, &exclude).await?;
            extend_inserted_keys(&mut out, res)?;
        }

        if let Some(rm) = chunks.into_remainder() {
            let res = DbOper::insert_return_key(&txn, rm.into_iter(), &conflict, &exclude).await?;
            extend_inserted_keys(&mut out, res)?;
        }
        Ok(out)
    }

    pub async fn insert_on_return_key<Et, A, I>(
        txn: &DatabaseTransaction,
        iter: I,
        conflict: OnConflict,
    ) -> Result<Vec<EntityKey<Et>>, sea_orm::DbErr>
    where
        Et: EntityTrait,
        Et::Model: sea_orm::ModelTrait + sea_orm::IntoActiveModel<A>,
        A: ActiveModelTrait<Entity = Et>,
        I: IntoIterator<Item = A>,
    {
        let mut out = Vec::new();
        let mut chunks = iter.into_iter().const_chunks::<N>();
        for iter in &mut chunks {
            let res = DbOper::insert_on_return_key(&txn, iter, conflict.clone()).await?;
            extend_inserted_keys(&mut out, res)?;
        }

        if let Some(rm) = chunks.into_remainder() {
            let res = DbOper::insert_on_return_key(&txn, rm.into_iter(), conflict).await?;
            extend_inserted_keys(&mut out, res)?;
        }

        Ok(out)
    }
}

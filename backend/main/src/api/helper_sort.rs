use crate::msg;
use qcm_core::model as sqlm;
use sea_orm::sea_query::Expr;
use sea_orm::{Order, QueryOrder};

pub fn order_songs<T: QueryOrder>(mut query: T, sort: msg::model::SongSort, order: Order) -> T {
    if sort == msg::model::SongSort::TrackNumber {
        query = query.order_by(
            Expr::col((sqlm::song::Entity, sqlm::song::Column::DiscNumber)),
            order.clone(),
        );
    }
    query.order_by(song_sort_col(sort), order.clone()).order_by(
        Expr::col((sqlm::song::Entity, sqlm::song::Column::Id)),
        order,
    )
}

pub fn song_sort_col(sort: msg::model::SongSort) -> Expr {
    use msg::model::SongSort;
    match sort {
        SongSort::PublishTime => Expr::col((sqlm::song::Entity, sqlm::song::Column::PublishTime)),
        SongSort::Title => Expr::col((sqlm::song::Entity, sqlm::song::Column::Name)),
        SongSort::SortTitle => Expr::col((sqlm::song::Entity, sqlm::song::Column::SortName)),
        SongSort::TrackNumber => Expr::col((sqlm::song::Entity, sqlm::song::Column::TrackNumber)),
        SongSort::Duration => Expr::col((sqlm::song::Entity, sqlm::song::Column::Duration)),
        SongSort::Popularity => Expr::col((sqlm::song::Entity, sqlm::song::Column::Popularity)),
        SongSort::Random => Expr::expr(Expr::cust("RANDOM()")),
    }
}

pub fn album_sort_col(sort: msg::model::AlbumSort) -> Expr {
    use msg::model::AlbumSort;
    match sort {
        AlbumSort::LastPlayedAt => {
            Expr::col((sqlm::dynamic::Entity, sqlm::dynamic::Column::LastPlayedAt))
        }
        AlbumSort::Year => Expr::col(sqlm::album::Column::PublishTime),
        AlbumSort::PublishTime => Expr::col(sqlm::album::Column::PublishTime),
        AlbumSort::Title => Expr::col(sqlm::album::Column::Name),
        AlbumSort::SortTitle => Expr::col(sqlm::album::Column::SortName),
        AlbumSort::TrackCount => Expr::col(sqlm::album::Column::TrackCount),
        AlbumSort::AddedTime => Expr::col(sqlm::album::Column::AddedAt),
        AlbumSort::DiscCount => Expr::col(sqlm::album::Column::DiscCount),
        AlbumSort::Random => Expr::expr(Expr::cust("RANDOM()")),
    }
}

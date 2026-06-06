pub mod model {
    include!(concat!(env!("OUT_DIR"), "/qcm.msg.model.rs"));
}
pub mod filter {
    include!(concat!(env!("OUT_DIR"), "/qcm.msg.filter.rs"));
}
include!(concat!(env!("OUT_DIR"), "/qcm.msg.rs"));

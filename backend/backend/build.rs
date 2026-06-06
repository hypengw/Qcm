use std::io::Result;
use std::{env, path::Path};
fn main() -> Result<()> {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR is not set");
    let proto_inc = Path::new(&manifest_dir).join("proto");
    let protos = ["proto/model.proto", "proto/filter.proto", "proto/message.proto"].map(|file|{
        Path::new(&manifest_dir).join(file)
    });

    for p in &protos {
        println!("cargo:rerun-if-changed={}", p.display());
    }
    println!("cargo:rerun-if-changed=build.rs");
    prost_build::compile_protos(&protos, &[proto_inc])?;
    Ok(())
}

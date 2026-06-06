
#[derive(Default)]
pub struct TableMetadata {
    pub name: String,
    pub fields: Vec<FieldMetadata>,
}

#[derive(Default)]
pub struct FieldMetadata {
    pub name: String,
    pub column_type: String,
    pub is_primary: bool,
    pub foreign_key: Option<String>,
    pub not_null: bool,
    pub is_json: bool,
}

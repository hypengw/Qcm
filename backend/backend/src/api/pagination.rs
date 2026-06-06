#[derive(Debug, Clone)]
pub struct PageParams {
    pub page: u64,
    pub page_size: u64,
}

impl PageParams {
    pub fn new(page: i32, page_size: i32) -> Self {
        Self {
            page: page.max(0) as u64,
            page_size: page_size.max(1) as u64,
        }
    }

    pub fn has_more(&self, total: u64) -> bool {
        self.page * self.page_size < total
    }
}

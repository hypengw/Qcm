use crossbeam_channel::{Receiver, Sender};
use std::collections::HashMap;
use std::future::Future;
use std::sync::{atomic::AtomicI64, Arc};
use std::thread;
use tokio::sync::oneshot;
use tokio::task::JoinHandle;

enum TaskOneshotEvent {
    Cancel,
}

struct Task {
    id: i64,
    handle: JoinHandle<()>,
    sender: Option<oneshot::Sender<TaskOneshotEvent>>,
    name: Option<String>,
    waiters: Vec<oneshot::Sender<()>>, // status: TaskStatus,
}

enum TaskManagerEvent {
    Add(Task),
    Progress,
    Cancel { id: i64 },
    End { id: i64 },
    Stop,
    Wait { id: i64, tx: oneshot::Sender<()> },
    // Pause,
}
pub struct TaskOper {
    id: i64,
    tx: Sender<TaskManagerEvent>,
}

#[derive(Debug)]
struct TaskManagerInner {
    id: AtomicI64,
}

#[derive(Debug, Clone)]
pub struct TaskManagerOper {
    inner: Arc<TaskManagerInner>,
    sender: Sender<TaskManagerEvent>,
}

impl TaskManagerOper {
    fn gen_id(&self) -> i64 {
        return self
            .inner
            .id
            .fetch_add(1, std::sync::atomic::Ordering::SeqCst);
    }

    fn add(&self, task: Task) {
        let _ = self.sender.send(TaskManagerEvent::Add(task));
    }

    pub async fn wait(&self, id: i64) {
        let (tx, rx) = oneshot::channel::<()>();
        let _ = self.sender.send(TaskManagerEvent::Wait { id, tx: tx });
        let _ = rx.await;
    }

    pub fn canel(&self, id: i64) {
        let _ = self.sender.send(TaskManagerEvent::Cancel { id });
    }

    pub fn stop(&self) {
        let _ = self.sender.send(TaskManagerEvent::Stop);
    }

    pub fn spawn<Fut>(&self, future: impl FnOnce(i64) -> Fut + Send + 'static) -> i64
    where
        Fut: Future<Output = ()> + Send,
    {
        let task_id = self.gen_id();
        let (tx, rx) = oneshot::channel();
        let mgr_tx = self.sender.clone();

        let handle = tokio::spawn(async move {
            tokio::select! {
                _ = future(task_id) => {},
                one = rx => match one {
                    Ok(_) => {}
                    Err(_) => {}
                }
            }

            let _ = mgr_tx.send(TaskManagerEvent::End { id: task_id });
        });

        let task = Task {
            id: task_id,
            handle,
            sender: Some(tx),
            name: None,
            waiters: Vec::new(),
        };
        self.add(task);
        task_id
    }
}

pub struct TaskManager {
    inner: Arc<TaskManagerInner>,
    receiver: Receiver<TaskManagerEvent>,
    tasks: HashMap<i64, Task>,
}

impl TaskManager {
    pub fn new() -> (TaskManagerOper, Self) {
        let (tx, rx) = crossbeam_channel::unbounded();
        let inner = Arc::new(TaskManagerInner {
            id: AtomicI64::new(0),
        });
        (
            TaskManagerOper {
                inner: inner.clone(),
                sender: tx,
            },
            Self {
                inner: inner,
                receiver: rx,
                tasks: HashMap::new(),
            },
        )
    }

    pub fn start(mut self) -> thread::JoinHandle<()> {
        thread::spawn(move || {
            self.process();
        })
    }

    pub fn process(&mut self) {
        while let Ok(event) = self.receiver.recv() {
            match event {
                TaskManagerEvent::Add(task) => {
                    self.tasks.insert(task.id, task);
                }
                TaskManagerEvent::Cancel { id } => {
                    if let Some(t) = self.tasks.get_mut(&id) {
                        if let Some(tx) = t.sender.take() {
                            let _ = tx.send(TaskOneshotEvent::Cancel);
                        }
                    }
                }
                TaskManagerEvent::End { id } => {
                    if let Some(t) = self.tasks.get_mut(&id) {
                        while let Some(w) = t.waiters.pop() {
                            let _ = w.send(());
                        }
                    }
                    self.tasks.remove(&id);
                }
                TaskManagerEvent::Wait { id, tx } => {
                    if let Some(t) = self.tasks.get_mut(&id) {
                        t.waiters.push(tx);
                    } else {
                        log::warn!("task not found: {}", id);
                        let _ = tx.send(());
                    }
                }
                TaskManagerEvent::Progress => {}
                TaskManagerEvent::Stop => {
                    break;
                }
            }
        }
    }
}

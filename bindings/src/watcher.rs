/*
 * This file is part of CuteCosmic.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
use std::{any::TypeId, cell::RefCell, ffi::c_void, rc::Rc, task::Poll};

use cosmic::{
    config::CosmicTk,
    cosmic_config::{self, CosmicConfigEntry},
    cosmic_theme::{Theme, ThemeMode},
    iced::{
        Executor, Subscription,
        futures::{
            Sink,
            channel::{mpsc::SendError, oneshot},
            executor::block_on,
            task::SpawnExt,
        },
    },
    iced_futures::subscription::into_recipes,
};
use cosmic_settings_daemon::CosmicSettingsDaemonProxy;

pub struct CosmicWatcherToken {
    stop_signal: oneshot::Sender<()>,
}

/// Very simple iced executor that does everything on the thread it is run on,
/// blocking it until notified to stop
#[derive(Clone)]
struct LocalExecutor {
    pool: Rc<RefCell<cosmic::iced::futures::executor::LocalPool>>,
}

impl LocalExecutor {
    fn run(&mut self, stop_rx: oneshot::Receiver<()>) {
        let _ = self.pool.borrow_mut().run_until(stop_rx);
    }
}

impl Executor for LocalExecutor {
    fn new() -> Result<Self, cosmic::iced::futures::io::Error>
    where
        Self: Sized,
    {
        Ok(Self {
            pool: Rc::new(RefCell::new(
                cosmic::iced::futures::executor::LocalPool::new(),
            )),
        })
    }

    fn spawn(&self, future: impl Future<Output = ()> + cosmic::iced_futures::MaybeSend + 'static) {
        self.pool.borrow().spawner().spawn(future).unwrap();
    }
}

/// Implementation of `Sink` that just invokes a FFI function pointer each time
/// a new item arrives
#[derive(Clone)]
struct CallbackSink {
    callback: extern "C" fn(*mut c_void) -> c_void,
    data: *mut c_void,
}

unsafe impl Send for CallbackSink {}

impl Sink<()> for CallbackSink {
    type Error = SendError;

    fn poll_ready(
        self: std::pin::Pin<&mut Self>,
        _cx: &mut std::task::Context<'_>,
    ) -> Poll<Result<(), Self::Error>> {
        Poll::Ready(Ok(()))
    }

    fn start_send(self: std::pin::Pin<&mut Self>, _item: ()) -> Result<(), Self::Error> {
        (self.callback)(self.data);
        Ok(())
    }

    fn poll_flush(
        self: std::pin::Pin<&mut Self>,
        _cx: &mut std::task::Context<'_>,
    ) -> Poll<Result<(), Self::Error>> {
        Poll::Ready(Ok(()))
    }

    fn poll_close(
        self: std::pin::Pin<&mut Self>,
        _cx: &mut std::task::Context<'_>,
    ) -> Poll<Result<(), Self::Error>> {
        Poll::Ready(Ok(()))
    }
}

fn watch_config<I, T>(
    proxy: Option<&CosmicSettingsDaemonProxy<'static>>,
    config_id: &'static str,
) -> Subscription<()>
where
    I: 'static,
    T: 'static + Send + Sync + PartialEq + Clone + Default + CosmicConfigEntry,
{
    let sub = if let Some(daemon) = proxy {
        cosmic_config::dbus::watcher_subscription::<T>(daemon.clone(), config_id, false)
    } else {
        cosmic_config::config_subscription::<_, T>(TypeId::of::<I>(), config_id.into(), T::VERSION)
    };

    // Since we don't care what exactly changed, just that something relevant
    // did - this erases the update type so we can watch all the configuration
    // entries we care about in one subscription
    sub.map(|_update| ())
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_watcher_start(
    callback: extern "C" fn(*mut c_void) -> c_void,
    data: *mut c_void,
) -> *mut CosmicWatcherToken {
    struct ThemeModeSubscription;
    struct DarkThemeSubscription;
    struct LightThemeSubscription;
    struct CosmicTkSubscription;

    let sender = CallbackSink { callback, data };
    let (stop_tx, stop_rx) = oneshot::channel::<()>();

    std::thread::Builder::new()
        .name("CuteCosmicWatcher".into())
        .spawn(move || {
            let proxy = block_on(cosmic_config::dbus::settings_daemon_proxy()).ok();

            let sub = Subscription::batch([
                watch_config::<ThemeModeSubscription, ThemeMode>(
                    proxy.as_ref(),
                    cosmic::cosmic_theme::THEME_MODE_ID,
                ),
                watch_config::<DarkThemeSubscription, Theme>(
                    proxy.as_ref(),
                    cosmic::cosmic_theme::DARK_THEME_ID,
                ),
                watch_config::<LightThemeSubscription, Theme>(
                    proxy.as_ref(),
                    cosmic::cosmic_theme::LIGHT_THEME_ID,
                ),
                watch_config::<CosmicTkSubscription, CosmicTk>(proxy.as_ref(), cosmic::config::ID),
            ]);

            let mut executor = LocalExecutor::new().unwrap();
            let mut rt: cosmic::iced_futures::Runtime<_, _, ()> =
                cosmic::iced_futures::Runtime::new(executor.clone(), sender);

            rt.track(into_recipes(sub));

            executor.run(stop_rx);
        })
        .unwrap();

    let token = CosmicWatcherToken {
        stop_signal: stop_tx,
    };
    Box::into_raw(Box::new(token))
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_watcher_stop(token: *mut CosmicWatcherToken) {
    let token = unsafe { Box::from_raw(token) };
    let _ = token.stop_signal.send(());
}

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
use atomic_refcell::{AtomicRef, AtomicRefCell};

static CURRENT_THEME: AtomicRefCell<Option<cosmic::theme::Theme>> = AtomicRefCell::new(None);

fn current_theme() -> AtomicRef<'static, cosmic::theme::Theme> {
    AtomicRef::map(CURRENT_THEME.borrow(), |o| {
        o.as_ref().expect("Theme not loaded")
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_load() {
    let theme = cosmic::theme::system_preference();

    *CURRENT_THEME.borrow_mut() = Some(theme);
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_is_dark() -> bool {
    return current_theme().cosmic().is_dark;
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_is_high_contrast() -> bool {
    return current_theme().cosmic().is_high_contrast;
}

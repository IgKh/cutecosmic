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
use std::ffi::{CString, c_char, c_int};

use atomic_refcell::{AtomicRef, AtomicRefCell};
use cosmic::{config::CosmicTk, cosmic_config::CosmicConfigEntry};

#[repr(C)]
#[allow(dead_code)]
pub enum CosmicThemeKind {
    SystemPreference,
    Dark,
    Light,
}

#[repr(C)]
#[allow(dead_code)]
pub enum CosmicFontKind {
    Interface,
    Monospace,
}

#[repr(C)]
pub enum CosmicFontStyle {
    Normal,
    Italic,
    Oblique,
}

#[repr(C)]
pub struct CosmicFont {
    family: *mut c_char,
    style: CosmicFontStyle,
    weight: c_int,
    stretch: c_int,
}

static CURRENT_THEME: AtomicRefCell<Option<cosmic::theme::Theme>> = AtomicRefCell::new(None);
static COSMIC_TK: AtomicRefCell<Option<CosmicTk>> = AtomicRefCell::new(None);

fn current_theme() -> AtomicRef<'static, cosmic::theme::Theme> {
    AtomicRef::map(CURRENT_THEME.borrow(), |o| {
        o.as_ref().expect("Theme not loaded")
    })
}

fn current_tk() -> AtomicRef<'static, CosmicTk> {
    AtomicRef::map(COSMIC_TK.borrow(), |o| {
        o.as_ref().expect("Toolkit configuration not loaded")
    })
}

fn strdup(value: &str) -> *mut c_char {
    if let Ok(value) = CString::new(value) {
        value.into_raw()
    } else {
        std::ptr::null_mut()
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn libcosmic_theme_free_string(ptr: *mut c_char) {
    if ptr.is_null() {
        return;
    }

    // SAFETY: We checked for a null pointer, and assume that the C++ code will
    // only call this function on pointers received from FFI functions in this
    // module only
    let _ = unsafe { CString::from_raw(ptr) };
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_load(kind: CosmicThemeKind) {
    let theme = match kind {
        CosmicThemeKind::SystemPreference => cosmic::theme::system_preference(),
        CosmicThemeKind::Dark => cosmic::theme::system_dark(),
        CosmicThemeKind::Light => cosmic::theme::system_light(),
    };

    let tk = CosmicTk::config()
        .ok()
        .and_then(|c| CosmicTk::get_entry(&c).ok())
        .unwrap_or_default();

    *CURRENT_THEME.borrow_mut() = Some(theme);
    *COSMIC_TK.borrow_mut() = Some(tk);
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_is_dark() -> bool {
    current_theme().cosmic().is_dark
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_is_high_contrast() -> bool {
    current_theme().cosmic().is_high_contrast
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_icon_theme() -> *mut c_char {
    let tk = current_tk();

    strdup(&tk.icon_theme)
}

#[unsafe(no_mangle)]
pub extern "C" fn libcosmic_theme_get_font(kind: CosmicFontKind, target: *mut CosmicFont) {
    if target.is_null() {
        return;
    }
    let target: &mut CosmicFont = unsafe { &mut *target };

    let tk = current_tk();
    let font = match kind {
        CosmicFontKind::Interface => &tk.interface_font,
        CosmicFontKind::Monospace => &tk.monospace_font,
    };

    target.family = strdup(&font.family);

    target.style = match font.style {
        cosmic::iced::font::Style::Normal => CosmicFontStyle::Normal,
        cosmic::iced::font::Style::Italic => CosmicFontStyle::Italic,
        cosmic::iced::font::Style::Oblique => CosmicFontStyle::Oblique,
    };

    // From https://doc.qt.io/qt-6/qfont.html#Weight-enum
    target.weight = match font.weight {
        cosmic::iced::font::Weight::Thin => 100,
        cosmic::iced::font::Weight::ExtraLight => 200,
        cosmic::iced::font::Weight::Light => 300,
        cosmic::iced::font::Weight::Normal => 400,
        cosmic::iced::font::Weight::Medium => 500,
        cosmic::iced::font::Weight::Semibold => 600,
        cosmic::iced::font::Weight::Bold => 700,
        cosmic::iced::font::Weight::ExtraBold => 800,
        cosmic::iced::font::Weight::Black => 900,
    };

    // From https://doc.qt.io/qt-6/qfont.html#Stretch-enum
    target.stretch = match font.stretch {
        cosmic::iced::font::Stretch::UltraCondensed => 50,
        cosmic::iced::font::Stretch::ExtraCondensed => 62,
        cosmic::iced::font::Stretch::Condensed => 75,
        cosmic::iced::font::Stretch::SemiCondensed => 87,
        cosmic::iced::font::Stretch::Normal => 100,
        cosmic::iced::font::Stretch::SemiExpanded => 112,
        cosmic::iced::font::Stretch::Expanded => 125,
        cosmic::iced::font::Stretch::ExtraExpanded => 150,
        cosmic::iced::font::Stretch::UltraExpanded => 200,
    };
}

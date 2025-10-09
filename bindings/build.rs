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
use cbindgen::Language;

fn main() {
    println!("cargo:rerun-if-changed=src/lib.rs");

    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let output_file = std::env::var("BINDINGS_HEADER_PATH").unwrap();

    cbindgen::Builder::new()
        .with_crate(manifest_dir)
        .with_language(Language::Cxx)
        .generate()
        .expect("Unable to generate libcosmic C++ bindings")
        .write_to_file(output_file);
}

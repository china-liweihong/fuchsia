// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use {
    anyhow::{format_err, Context as _, Error},
    fidl_fuchsia_media::AudioRenderUsage,
    fidl_fuchsia_media_sounds::PlayerProxy,
    fuchsia_syslog::{fx_log_err, fx_log_info},
    fuchsia_zircon::{self as zx},
    futures::lock::Mutex,
    std::collections::HashSet,
    std::fs::File,
    std::sync::Arc,
};

/// Creates a file-based sound from a resource file.
fn resource_file(
    name: &str,
) -> Result<fidl::endpoints::ClientEnd<fidl_fuchsia_io::FileMarker>, Error> {
    // We try two paths here, because normal components see their config package data resources in
    // /pkg/data and shell tools see them in /pkgfs/packages/config-data/0/data/<pkg>.
    Ok(fidl::endpoints::ClientEnd::<fidl_fuchsia_io::FileMarker>::new(zx::Channel::from(
        fdio::transfer_fd(
            File::open(format!("/config/data/{}", name))
                .or_else(|_| {
                    File::open(format!("/pkgfs/packages/config-data/0/data/setui_service/{}", name))
                })
                .context("Opening package data file")?,
        )?,
    )))
}

/// Plays a sound with the given [id] and [file_name] via the [sound_player_proxy].
///
/// The id and file_name are expected to be unique and mapped 1:1 to each other. This allows
/// the sound file to be reused without having to load it again.
pub async fn play_sound<'a>(
    sound_player_proxy: &PlayerProxy,
    file_name: &'a str,
    id: u32,
    added_files: Arc<Mutex<HashSet<&'a str>>>,
) -> Result<(), Error> {
    // New sound, add it to the sound player set.
    if added_files.lock().await.insert(file_name) {
        let sound_file_channel = match resource_file(file_name) {
            Ok(file) => Some(file),
            Err(e) => return Err(format_err!("[earcons] Failed to convert sound file: {}", e)),
        };
        if let Some(file_channel) = sound_file_channel {
            match sound_player_proxy.add_sound_from_file(id, file_channel).await {
                Ok(_) => fx_log_info!("[earcons] Added sound to Player: {}", file_name),
                Err(e) => {
                    return Err(format_err!("[earcons] Unable to add sound to Player: {}", e));
                }
            };
        }
    }

    match sound_player_proxy.play_sound(id, AudioRenderUsage::Media).await {
        Ok(_) => {
            // TODO(fxb/50246): Add inspect logging.
        }
        Err(e) => fx_log_err!("[earcons] Unable to Play sound from Player: {}", e),
    };
    Ok(())
}

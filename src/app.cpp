#include "app.hpp"

#include "clipboard.hpp"
#include "notify.hpp"
#include "util.hpp"
#include "wsl.hpp"

#include <string>


std::optional<Acquired> acquire_image() {
    ClipContents clip = ClipContents::read();

    if (clip.has_png()) {
        if (auto image = Image::decode(clip.png)) {
            return Acquired{ .image = std::move(*image), .was_file_drop = false };
        }
        return {};
    }
    if (clip.has_image()) {
        if (auto image = Image::from_dib(clip.dib)) {
            return Acquired{ .image = std::move(*image), .was_file_drop = false };
        }
        return {};
    }
    if (auto file = clip.first_image_file()) {
        if (auto image = Image::decode(read_file(*file))) {
            return Acquired{
                .image = std::move(*image),
                .was_file_drop = true,
                .files = std::move(clip.files)
            };
        }
    }
    return {};
}

int run(HINSTANCE hInst, const Config& cfg) {
    auto acquired = acquire_image();
    if (not acquired) {
        error_alert(hInst, L"No image on clipboard [E1]");
        return 1;
    }
    auto& [image, was_file_drop, files] = *acquired;

    image.cap_to(cfg.max_dim);
    const auto png = image.encode_png();
    if (not png) {
        error_alert(hInst, L"Could not encode PNG [E2]");
        return 1;
    }

    // A file drop leaves a text path on the clipboard, so swap in the image;
    // direct image data is already there. This stops Terminal's Ctrl+V pasting text.
    if (was_file_drop) {
        set_clipboard(CF_DIB, image.to_dib());
    }

    // Stream the PNG to the WSL serve script, which puts it on the X11 clipboard.
    if (const int rc = wsl::run(cfg.serve_script, *png); rc != 0) {
        if (was_file_drop) {
            set_clipboard(CF_HDROP, make_drop_handle(files));
        }
        error_alert(hInst, L"Serve failed [E3 rc=" + std::to_wstring(rc) + L"]");
        return 1;
    }

    if (was_file_drop) {
        Sleep(cfg.settle_ms);
    }
    send_ctrl_v();

    if (was_file_drop) {
        Sleep(cfg.restore_ms);
        set_clipboard(CF_HDROP, make_drop_handle(files));
    }
    return 0;
}

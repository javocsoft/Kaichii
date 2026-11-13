// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 Torzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shared_translation.h"

#include <map>
#include <memory>
#include <utility>
#include <QCoreApplication>
#include "common/settings.h"
#include "common/settings_enums.h"
#include "common/settings_setting.h"
#include "common/time_zone.h"
#include "qt_common/config/uisettings.h"

namespace ConfigurationShared {

/**
 * Builds the label and tooltip every settings widget shows, keyed by setting id.
 *
 * @param parent Object used for translation lookups.
 * @return The translation map.
 */
std::unique_ptr<TranslationMap> InitializeTranslations(QObject* parent) {
    std::unique_ptr<TranslationMap> translations = std::make_unique<TranslationMap>();
    const auto& tr = [](const char* text) -> QString { return QCoreApplication::translate("ConfigurationShared", text); };

#define INSERT(SETTINGS, ID, NAME, TOOLTIP)                                                        \
    translations->insert(std::pair{SETTINGS::values.ID.Id(), std::pair{(NAME), (TOOLTIP)}})

    // A setting can be ignored by giving it a blank name

    // Applets
    const QString applet_modes =
        tr("Custom frontend: Kaichii answers in the applet's place, with its own window where it "
           "has one, and needs no firmware. Real applet: the console's own screen, from the "
           "installed firmware.");
    INSERT(Settings, cabinet_applet_mode, tr("Amiibo editor"),
           QString(tr("The screen where a game registers, renames or erases an amiibo.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, controller_applet_mode, tr("Controller configuration"),
           QString(tr("The screen where a game asks which controllers to use. Disabled skips it, "
                      "and the game keeps the controllers already connected.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, data_erase_applet_mode, tr("Data erase"),
           QString(tr("The screen for erasing a game's save data.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, error_applet_mode, tr("Error"),
           QString(tr("How the error messages a game raises are shown.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, net_connect_applet_mode, tr("Net connect"),
           QString(tr("The screen a game opens to connect to the internet.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, player_select_applet_mode, tr("Player select"),
           QString(tr("The screen where a game asks which user profile plays.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, swkbd_applet_mode, tr("Software keyboard"),
           QString(tr("The on-screen keyboard a game opens to type text.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, mii_edit_applet_mode, tr("Mii Edit"),
           QString(tr("The Mii editor a game opens.") + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, web_applet_mode, tr("Online web"),
           QString(tr("The browser a game opens for online pages.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, shop_applet_mode, tr("Shop"),
           QString(tr("The Nintendo eShop pages a game opens.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, photo_viewer_applet_mode, tr("Photo viewer"),
           QString(tr("The album a game opens to show its screenshots.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, offline_web_applet_mode, tr("Offline web"),
           QString(tr("The browser a game opens for its manual and the other pages stored with it.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, login_share_applet_mode, tr("Login share"),
           QString(tr("The Nintendo Account login a game opens.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, wifi_web_auth_applet_mode, tr("Wifi web auth"),
           QString(tr("The page for logging in to a network that asks for it.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, my_page_applet_mode, tr("My page"),
           QString(tr("The user page, with friends and profile, that a game opens.")
                   + QLatin1Char('\n') + applet_modes));
    INSERT(Settings, enable_overlay, tr("Console overlay menu (hold Home)"),
           tr("Enables the console's own overlay menu (Horizon's overlay applet), from the "
              "installed firmware. Hold the Home button for one second to show it."));

    // Audio
    INSERT(Settings, sink_id, tr("Output Engine:"),
           tr("The system Kaichii plays sound through. Auto picks one that works here; change it "
              "only if there is no sound or it crackles. Null plays nothing."));
    INSERT(Settings, audio_output_device_id, tr("Output Device:"),
           tr("Where the game's sound plays. Auto follows your system's default output."));
    INSERT(Settings, audio_input_device_id, tr("Input Device:"),
           tr("The microphone games hear. Auto uses your system's default input."));
    INSERT(Settings, audio_muted, tr("Mute audio"),
           tr("Silences the game."));
    INSERT(Settings, volume, tr("Volume:"),
           tr("The game's volume."));
    INSERT(Settings, dump_audio_commands, QString(), QString());
    INSERT(UISettings, mute_when_in_background, tr("Mute audio when in background"),
           tr("Silences the game while another window has the focus."));

    // Core
    INSERT(Settings, use_multi_core, tr("Multicore CPU Emulation"),
           tr("Runs the console's processor cores on several threads of your processor. Off makes "
              "games far slower; it is only for finding emulator bugs."));
    INSERT(Settings, memory_layout_mode, tr("Memory Layout"),
           tr("Increases the amount of emulated RAM.\nDoesn't affect performance/stability but may "
              "allow HD texture "
              "mods to load."));
    INSERT(Settings, use_speed_limit, QString(), QString());
    INSERT(Settings, current_speed_mode, QString(), QString());
    INSERT(Settings, speed_limit, tr("Limit Speed Percent"),
           tr("Controls the game's maximum rendering speed, but it's up to each game if it runs "
              "faster or not.\n200% for a 30 FPS game is 60 FPS, and for a "
              "60 FPS game it will be 120 FPS.\nDisabling it means unlocking the framerate to the "
              "maximum your PC can reach."));

    INSERT(Settings, turbo_speed_limit, tr("Turbo Speed"),
           tr("When the Turbo Speed hotkey is pressed, the speed will be limited to this "
              "percentage."));
    INSERT(Settings, slow_speed_limit, tr("Slow Speed"),
           tr("When the Slow Speed hotkey is pressed, the speed will be limited to this "
              "percentage."));

    INSERT(Settings, sync_core_speed, tr("Synchronize Core Speed"),
           tr("Synchronizes CPU core speed with the game's maximum rendering speed to boost FPS "
              "without affecting game speed (animations, physics, etc.).\n"
              "Can help reduce stuttering at lower framerates."));

    // Cpu
    INSERT(Settings, cpu_accuracy, tr("Accuracy:"),
           tr("How faithfully the console's processor is emulated. Auto suits nearly every game, "
              "and Accurate is slower. Unsafe allows the speed-ups below, which bend the rules. "
              "Paranoid and Debugging are for finding emulator bugs."));
    INSERT(Settings, cpu_backend, tr("Backend:"),
           tr("What runs the game's processor code. Dynarmic translates it and works on every "
              "processor; native code execution runs it directly, on ARM64 processors only."));

    INSERT(Settings, cpu_clock, tr("CPU Clocks"),
           tr("Raises the clock the emulated CPU reports, which removes some FPS limiters.\n"
              "Weaker CPUs may see reduced performance, and certain games may behave improperly."));

    INSERT(Settings, use_custom_cpu_ticks, QString(), QString());
    INSERT(Settings, cpu_ticks, tr("Custom CPU Ticks"),
           tr("A custom count of CPU ticks for the emulated processor. Higher values can raise "
              "performance but can freeze games. A range of 77-21000 is recommended."));

    // Cpu Debug

    // Cpu Unsafe
    INSERT(
        Settings, cpuopt_unsafe_host_mmu, tr("Enable Host MMU Emulation (fastmem)"),
        tr("This optimization speeds up memory accesses by the guest program.\nEnabling it causes "
           "guest memory reads/writes to be done directly into memory and make use of Host's "
           "MMU.\nDisabling this forces all memory accesses to use Software MMU Emulation."));
    INSERT(
        Settings, cpuopt_unsafe_unfuse_fma,
        tr("Unfuse FMA (improve performance on CPUs without FMA)"),
        tr("This option improves speed by reducing accuracy of fused-multiply-add instructions on "
           "CPUs without native FMA support."));
    INSERT(
        Settings, cpuopt_unsafe_reduce_fp_error, tr("Faster FRSQRTE and FRECPE"),
        tr("This option improves the speed of some approximate floating-point functions by using "
           "less accurate native approximations."));
    INSERT(Settings, cpuopt_unsafe_ignore_standard_fpcr,
           tr("Faster ASIMD instructions (32 bits only)"),
           tr("This option improves the speed of 32 bits ASIMD floating-point functions by running "
              "with incorrect rounding modes."));
    INSERT(Settings, cpuopt_unsafe_inaccurate_nan, tr("Inaccurate NaN handling"),
           tr("This option improves speed by removing NaN checking.\nPlease note this also reduces "
              "accuracy of certain floating-point instructions."));
    INSERT(Settings, cpuopt_unsafe_fastmem_check, tr("Disable address space checks"),
           tr("This option improves speed by eliminating a safety check before every memory "
              "operation.\nDisabling it may allow arbitrary code execution."));
    INSERT(
        Settings, cpuopt_unsafe_ignore_global_monitor, tr("Ignore global monitor"),
        tr("This option improves speed by relying only on the semantics of cmpxchg to ensure "
           "safety of exclusive access instructions.\nPlease note this may result in deadlocks and "
           "other race conditions."));

    // Renderer
    INSERT(Settings, renderer_backend, tr("API:"),
           tr("The graphics API Kaichii draws with. Vulkan is recommended."));
    INSERT(Settings, vulkan_device, tr("Device:"),
           tr("The graphics card Kaichii draws with, when the computer has more than one."));
    INSERT(Settings, resolution_setup, tr("Resolution:"),
           tr("The size games are drawn at, as a multiple of their size on the console. Higher is "
              "sharper but needs a faster graphics card and more video memory. Below 1x is faster "
              "but blurrier and can show artifacts; a window adapting filter such as FSR 1 still "
              "fills the window."));
    INSERT(Settings, scaling_filter, tr("Window Adapting Filter:"),
           tr("How the image is resized to fit the window. FSR 1 and SGSR sharpen as they enlarge, "
              "which suits resolutions below 1x; FSR 1 Soft sharpens less. Bilinear and Bicubic "
              "are smooth, Nearest keeps hard pixel edges, and ScaleForce and MMPX suit pixel "
              "art."));
    INSERT(Settings, game_dlss_network, tr("Use the game's own DLSS"),
           tr("Runs a Switch 2 game's own DLSS (its upscaling network, translated for this GPU) "
              "instead of AMD FSR 3.1 in its place.\nAMD FSR 3.1 still draws while the network is "
              "checked, when it can't run on this GPU, with resolution scaling, and for the rest "
              "of the session if its output fails Kaichii's checks."));
    INSERT(Settings, fsr_sharpening_slider, tr("Sharpening:"),
           tr("How strongly FSR 1, FSR 1 Soft and SGSR sharpen the image as they resize it, and "
              "AMD FSR 3.1 where it stands in for a game's DLSS. 0% turns sharpening off."));
    INSERT(Settings, nvidia_dlss, tr("Use NVIDIA DLSS in place of a game's DLSS (experimental)"),
           tr("Uses NVIDIA DLSS on GeForce RTX cards in place of a Switch 2 game's own DLSS, "
              "instead of AMD FSR 3.1.\nNeeds NVIDIA's Streamline loader, and NVIDIA DLSS unless "
              "the NVIDIA app has downloaded it; Kaichii does not ship NVIDIA's files. Use the "
              "download button below.\nFire Emblem: Fortune's Weave also needs "
              "EDEN_NX2_DLSS_REPLACEMENT=dlss. Without that override, its spatial workaround "
              "remains active.\nIn The Legend of Zelda: Ocarina of Time it replaces the game's "
              "upscaler at 1x resolution scale."));
    INSERT(Settings, anti_aliasing, tr("Anti-Aliasing Method:"),
           tr("Smooths jagged edges in the final image. SMAA looks best; FXAA is lighter and "
              "steadier at low resolutions."));
    INSERT(Settings, oot_anti_aliasing, tr("Ocarina of Time anti-aliasing:"),
           tr("Game: the game's own anti-aliasing network, which sharpens heavily.\nNo sharpening: "
              "the image as the game renders it, without the network (faster; pairs well with a "
              "higher resolution). Has no effect while NVIDIA DLSS is on."));
    INSERT(Settings, post_shader_chain, QString(), QString());
    INSERT(Settings, post_shader_preset, QString(), QString());
    INSERT(Settings, post_shader_enabled, tr("Enable post-processing effects"),
           tr("Applies the effects chosen under Configure Effects to the final image."));
    INSERT(Settings, fullscreen_mode, tr("Fullscreen Mode:"),
           tr("The method used to render the window in fullscreen.\nBorderless offers the best "
              "compatibility with the on-screen keyboard that some games request for "
              "input.\nExclusive "
              "fullscreen may offer better performance and better Freesync/Gsync support."));
    INSERT(Settings, aspect_ratio, tr("Aspect Ratio:"),
           tr("Stretches the renderer to fit the specified aspect ratio.\nMost games only support "
              "16:9, so modifications are required to get other ratios.\nAlso controls the "
              "aspect ratio of captured screenshots."));
    INSERT(Settings, use_disk_shader_cache, tr("Use persistent pipeline cache"),
           tr("Saves the shaders each game builds, so later sessions load them instead of building "
              "them again while you play. Turn it off only to debug."));
    INSERT(
        Settings, use_asynchronous_gpu_emulation, tr("Use asynchronous GPU emulation"),
        tr("Uses an extra CPU thread for rendering.\nThis option should always remain enabled."));
    INSERT(Settings, nvdec_emulation, tr("NVDEC emulation:"),
           tr("Specifies how videos should be decoded.\nIt can either use the CPU or the GPU for "
              "decoding, or perform no decoding at all (black screen on videos).\n"
              "In most cases, GPU decoding provides the best performance."));
    INSERT(Settings, accelerate_astc, tr("ASTC Decoding Method:"),
           tr("How the game's ASTC textures are unpacked, since PC graphics cards can't read them "
              "directly.\nGPU: on the graphics card.\nCPU: on the processor, while the game "
              "waits.\nCPU Asynchronous: on the processor in the background, which "
              "removes the stutter but can show a texture blurry or missing for a moment."));
    INSERT(Settings, astc_recompression, tr("ASTC Recompression Method:"),
           tr("Unpacked ASTC textures take four bytes of video memory a pixel. BC3 packs them "
              "again into a quarter of that with a small loss of quality, BC1 into an eighth with "
              "a visible one. Uncompressed keeps full quality."));
    INSERT(Settings, frame_pacing_mode, tr("Frame Pacing Mode (Vulkan only)"),
           tr("Controls how the emulator manages frame pacing to reduce stuttering and make the "
              "frame rate smoother and more consistent."));
    INSERT(Settings, vram_usage_mode, tr("VRAM Usage Mode:"),
           tr("Whether Kaichii keeps its use of video memory low or uses as much as the card has, "
              "for speed. Aggressive can slow other programs that use the graphics card, such as "
              "recording software."));
    INSERT(Settings, skip_cpu_inner_invalidation, tr("Skip CPU Inner Invalidation"),
           tr("Skips clearing some of the GPU's cached copies of memory when the game writes to "
              "it. Uses less processor time, but the GPU can then use stale data: wrong images, or "
              "a game that freezes."));
    INSERT(Settings, fps_limit, tr("FPS Limit"),
           tr("Caps how often the game presents, so the game itself runs at this rate. 0 leaves "
              "the frame rate to the game and the display."));
    INSERT(Settings, vsync_mode, tr("VSync Mode:"),
           tr("FIFO (VSync) does not drop frames or exhibit tearing but is limited by the screen "
              "refresh rate.\nFIFO Relaxed allows tearing as it recovers from a slow down.\n"
              "Mailbox can have lower latency than FIFO and does not tear but may drop "
              "frames.\nImmediate (no synchronization) presents whatever is available and can "
              "exhibit tearing."));
    INSERT(Settings, emulate_bgr565, QString(), QString()); // Android only
    INSERT(Settings, bg_red, QString(), QString());
    INSERT(Settings, bg_green, QString(), QString());
    INSERT(Settings, bg_blue, QString(), QString());

    INSERT(Settings, frame_gen, tr("Enable Frame Generation"),
           tr("Draws extra frames between the game's own from the motion between them, so movement "
              "looks smoother. Costs some of the game's own frame rate and adds input lag; works "
              "best when the game already reaches about 30 fps. Optimize for This Game (Emulation "
              "menu) measures whether it pays off for a game."));
    INSERT(Settings, frame_gen_type, tr("Frame Generation Type"),
           tr("Optical Flow is built in. LSFG uses Lossless Scaling's frame generation and needs "
              "Lossless.dll from Lossless Scaling in Kaichii's \"lossless\" folder."));
    INSERT(Settings, frame_gen_multiplier, tr("Frame Generation Multiplier"),
           tr("How many frames reach the screen for each one the game draws: 2x adds one, 3x two, "
              "4x three. More is smoother but adds lag and artifacts."));
    INSERT(Settings, frame_gen_target_rate, tr("Target Frame Rate"),
           tr("Adds as many frames as it takes to reach this frame rate, up to 4x, instead of a "
              "fixed multiplier. 0 uses the multiplier."));
    INSERT(Settings, frame_gen_flow_scale_auto, tr("Auto Flow Scale"),
           tr("Estimates motion at the resolution the game renders at, relative to the window. Off "
              "uses the Flow Scale below."));
    INSERT(Settings, frame_gen_flow_scale, tr("Flow Scale"),
           tr("The resolution motion is estimated at, as a share of the window's. Lower is faster "
              "but less precise around moving edges."));
    INSERT(Settings, frame_gen_queue_target, tr("Queue Target"),
           tr("How many finished frames wait for the screen. More can make pacing steadier but "
              "adds lag; 0 keeps lag lowest."));
    INSERT(Settings, frame_gen_dump_flow, tr("Dump Optical Flow"),
           tr("Writes the motion frame generation estimates to image files, to debug it."));

    // Renderer (Advanced Graphics)
    INSERT(Settings, use_asynchronous_gpu_emulation, QString(), QString());

    INSERT(Settings, sync_memory_operations, tr("Sync Memory Operations"),
           tr("Ensures data consistency between compute and memory operations.\nThis option fixes "
              "issues in games, but may degrade performance.\nUnreal Engine 4 games often see the "
              "most significant changes thereof."));
    INSERT(Settings, async_presentation, tr("Enable asynchronous presentation (Vulkan only)"),
           tr("Slightly improves performance by moving presentation to a separate CPU thread."));
    INSERT(
        Settings, renderer_force_max_clock, tr("Force maximum clocks (Vulkan only)"),
        tr("Runs work in the background while waiting for graphics commands to keep the GPU from "
           "lowering its clock speed."));
    INSERT(Settings, max_anisotropy, tr("Anisotropic Filtering:"),
           tr("Controls the quality of texture rendering at oblique angles.\nSafe to set at 16x on "
              "most GPUs."));
    INSERT(Settings, gpu_accuracy, tr("GPU Mode:"),
           tr("Controls the GPU emulation mode.\nMost games render fine with Fast, but Accurate is still "
              "required for some.\nParticles tend to only render correctly with Accurate mode."));
    INSERT(Settings, dma_accuracy, tr("DMA Accuracy:"),
           tr("Whether the emulated GPU waits for its earlier work before it reads the game's "
              "commands and data. Safe waits, which can fix corrupted images in some games; Unsafe "
              "doesn't, and is faster. Default follows GPU Mode."));
    INSERT(Settings, gpu_fence_behavior, tr("GPU Fence Behavior:"),
           tr("When the game is told its GPU work is done. Immediate tells it at once: fastest, "
              "but it can cause flickering or wrong images. Balanced, and Accurate even more, wait "
              "for the graphics card first, which costs some speed. Default follows GPU Mode."));
    INSERT(Settings, native_compute_write_mark, tr("Native compute write mark:"),
           tr("How much of a Switch 2 compute shader's storage buffer is treated as written by "
              "the GPU after each dispatch.\nAuto marks what the shader's stores can reach and "
              "falls back to Full when that cannot be proven.\nFull marks the whole binding (up to "
              "8 MiB), the original behaviour: it can delay the game's own writes to nearby memory "
              "(flickering effects) and copy far more data back from the GPU.\n64 KiB cap marks at "
              "most 64 KiB, which can miss data a large dispatch really wrote."));
    INSERT(Settings, use_asynchronous_shaders, tr("Enable asynchronous shader compilation"),
           tr("Builds new shaders in the background while the game keeps running, instead of "
              "pausing it. Removes most of the stutter of new effects, but what they draw can be "
              "missing for a moment."));
    INSERT(Settings, gpu_clock, tr("GPU Clocks"),
           tr("Makes the game believe GPU work finishes faster than it does, so it stops lowering "
              "resolution and render distance to fit the Switch's clocks."));
    INSERT(Settings, gpu_unswizzle_enabled, tr("GPU Unswizzle"),
           tr("Rearranges compressed (BCn) 3D textures into the graphics card's layout on the card "
              "itself instead of on the processor. Turn it off if a game crashes or shows broken "
              "textures."));
    INSERT(Settings, gpu_unswizzle_texture_size, tr("GPU Unswizzle Max Texture Size"),
           tr("Sets the maximum size (MiB) for GPU-based texture unswizzling.\n"
              "While the GPU is faster for medium and large textures, the CPU may be more "
              "efficient for very small ones.\n"
              "Adjust this to find the balance between GPU acceleration and CPU overhead."));
    INSERT(Settings, gpu_unswizzle_stream_size, tr("GPU Unswizzle Stream Size"),
           tr("Sets the maximum amount of texture data (in MiB) processed per frame.\n"
              "Higher values can reduce stutter during texture loading but may impact frame "
              "consistency."));
    INSERT(Settings, gpu_unswizzle_chunk_size, tr("GPU Unswizzle Chunk Size"),
           tr("Determines the number of depth slices processed in a single dispatch.\n"
              "Increasing this can improve throughput on high-end GPUs but may cause TDR or driver "
              "timeouts on weaker hardware."));
    INSERT(Settings, pin_gpu_threads, tr("Pin GPU threads to their own cores (experimental)"),
           tr("Pins the emulator's GPU threads to dedicated physical CPU cores and keeps the "
              "emulated CPU cores off them.\n"
              "Can hurt performance on CPUs with few cores. Skipped on CPUs with fewer than 6 "
              "physical performance cores.\n"
              "Windows and Linux only."));

    INSERT(Settings, use_vulkan_driver_pipeline_cache, tr("Use Vulkan pipeline cache"),
           tr("Enables GPU vendor-specific pipeline cache.\nThis option can improve shader loading "
              "time significantly in cases where the Vulkan driver does not store pipeline cache "
              "files internally."));
    INSERT(Settings, native_async_draws, tr("Switch 2 draws with unbuilt pipelines:"),
           tr("Only applies when asynchronous shader building is on. Decides what a Switch 2 draw "
              "does when its graphics pipeline is still building.\n"
              "Wait stalls the GPU until the pipeline is ready (no missing effects, but long "
              "stalls on new content). Skip drops the draw, which can leave an effect missing for "
              "a few frames. Bounded waits up to the time below per frame, then skips. With Skip "
              "and Bounded, a new pipeline is also translated on a worker thread instead of the "
              "GPU thread, and the draws that may be skipped are skipped meanwhile. The "
              "EDEN_NX2_ASYNC_DISCARD=1 environment variable still forces Skip."));
    INSERT(Settings, shader_preload_memory, tr("Memory for preloaded shaders:"),
           tr("After a game starts, builds in the background the shaders earlier sessions used "
              "but the last few did not, most recent first, up to this much system memory. Areas "
              "you come back to then stutter less the first time. Off builds only the shaders "
              "of the last few sessions."));
    INSERT(Settings, native_pipeline_wait_ms, tr("Pipeline wait per frame (ms):"),
           tr("Used by the Bounded mode: the most time per frame that Switch 2 draws may wait in "
              "total for pipelines that are still building. Once spent, the remaining unbuilt "
              "draws of that frame are skipped."));
    INSERT(Settings, enable_compute_pipelines, tr("Enable Compute Pipelines (Intel Vulkan Only)"),
           tr("Required by some games.\nThis setting only exists for Intel "
              "proprietary drivers and may crash if enabled.\nCompute pipelines are always enabled "
              "on all other drivers."));
    INSERT(Settings, use_reactive_flushing, tr("Enable Reactive Flushing"),
           tr("Copies the images the GPU draws back to the game's memory only when the game reads "
              "them, instead of every time. Faster; turn it off only if an image the game reads "
              "back comes out wrong."));
    INSERT(Settings, use_video_framerate, tr("Sync to framerate of video playback"),
           tr("Run the game at normal speed during video playback, even when the framerate is "
              "unlocked."));
    INSERT(Settings, mkw_30fps, tr("Mario Kart World: 30 FPS mode"),
           tr("Forces the game to use its native 30 FPS mode at normal game speed."));
    INSERT(Settings, mkw_dynamic_fps, tr("Mario Kart World: dynamic FPS (experimental)"),
           tr("Lets the game adjust between 30 and 60 FPS using whole-frame timing to maintain "
              "normal game speed. Takes precedence over the 30 FPS mode."));
    INSERT(Settings, barrier_feedback_loops, tr("Barrier feedback loops"),
           tr("Makes the GPU finish writing a depth image before a draw reads it back, which fixes "
              "some transparency effects at a small cost."));
    INSERT(Settings, enable_buffer_history, tr("Enable buffer history"),
           tr("Records when each frame was queued and shown, for games that ask the system for "
              "that history. Some of them pace their frames better with it."));
    INSERT(Settings, fix_bloom_effects, tr("Fix bloom effects"),
           tr("Removes the bloom glow in Burnout Paradise Remastered, which renders wrongly "
              "there."));

    // Renderer (Extensions)
    INSERT(Settings, dyna_state, tr("Extended Dynamic State"),
           tr("Controls the number of features that can be used in Extended Dynamic State.\n"
              "Higher states allow for more features and can increase performance, but may cause "
              "additional graphical issues."));

    INSERT(Settings, vertex_input_dynamic_state, tr("Vertex Input Dynamic State"),
           tr("Lets the graphics card change the layout of vertex data without building a new "
              "pipeline: fewer pipelines and less stutter. Turn it off if a driver draws models "
              "wrong."));

    INSERT(
        Settings, sample_shading, tr("Sample Shading"),
        tr("Allows the fragment shader to execute per sample in a multi-sampled fragment "
           "instead of once per fragment. Improves graphics quality at the cost of performance.\n"
           "Higher values improve quality but degrade performance."));

    // Renderer (Debug)

    // System
    INSERT(Settings, rng_seed, tr("RNG Seed"),
           tr("Controls the seed of the random number generator.\nMainly used for speedrunning."));
    INSERT(Settings, rng_seed_enabled, QString(), QString());
    INSERT(Settings, device_name, tr("Device Name"),
           tr("The console's name, as games and local multiplayer show it."));
    INSERT(Settings, program_args, tr("Homebrew Args"),
           tr("Command-line arguments passed to homebrew at launch (e.g. -noglsl)."));
    INSERT(Settings, custom_rtc, tr("Custom RTC Date:"),
           tr("Sets the console's clock to this date and time instead of your computer's. Some "
              "games use the clock for daily events."));
    INSERT(Settings, custom_rtc_enabled, QString(), QString());
    INSERT(Settings, custom_rtc_offset, QStringLiteral(" "),
           tr("How far the console's clock is from your computer's, in seconds."));
    INSERT(Settings, language_index, tr("Language:"),
           tr("The language the console reports to games. With Region on Auto-select, the game's "
              "region can override it."));
    INSERT(Settings, region_index, tr("Region:"), tr("The region of the console."));
    INSERT(Settings, time_zone_index, tr("Time Zone:"), tr("The time zone of the console."));
    INSERT(Settings, sound_index, tr("Sound Output Mode:"),
           tr("The speakers the console tells games it has: mono, stereo or surround. Games mix "
              "their sound for them."));
    INSERT(Settings, use_docked_mode, tr("Console Mode:"),
           tr("Selects if the console is in Docked or Handheld mode.\nGames will change "
              "their resolution, details and supported controllers and depending on this setting.\n"
              "Setting to Handheld can help improve performance for low end systems."));
    INSERT(Settings, current_user, QString(), QString());

    // Controls

    // Data Storage

    // Debugging

    // Debugging Graphics

    // Network

    // Web Service

    // Ui

    // Ui General
    INSERT(UISettings, select_user_on_boot, tr("Prompt for user profile on boot"),
           tr("Useful if multiple people use the same PC."));
    INSERT(UISettings, pause_when_in_background, tr("Pause when not in focus"),
           tr("Pauses emulation when focusing on other windows."));
    INSERT(UISettings, confirm_before_stopping, tr("Confirm before stopping emulation"),
           tr("Whether Kaichii asks before it stops a running game. \"Only if game specifies not "
              "to stop\" asks only while the game says stopping now could lose data, such as "
              "during a save."));
    INSERT(UISettings, hide_mouse, tr("Hide mouse on inactivity"),
           tr("Hides the mouse after 2.5s of inactivity."));
    INSERT(UISettings, check_for_updates, tr("Check for updates"),
           tr("Whether or not to check for updates upon startup."));

    // Linux
    INSERT(UISettings, enable_gamemode, tr("Enable Gamemode"),
           tr("Asks Feral GameMode, when it is installed, to switch the system to its performance "
              "settings while a game runs."));
#ifdef __unix__
#ifdef __linux__
    INSERT(UISettings, desktop_integration, tr("Add Kaichii to the applications menu"),
           tr("When Kaichii runs as an AppImage, it adds itself to your desktop's applications "
              "menu, so you can start it from there and the taskbar shows its icon. Turning it off "
              "removes the entry."));
#endif
    INSERT(UISettings, gui_force_x11, tr("Force X11 as Graphics Backend"),
           tr("Runs Kaichii's windows through X11 (XWayland) instead of Wayland. On by default "
              "because under Wayland some desktops cut message boxes short. Takes effect when "
              "Kaichii restarts."));
    INSERT(UISettings, gui_hide_backend_warning, QString(), QString());
#endif

    // Ui Debugging

    // Ui Multiplayer

    // Ui Games list

#undef INSERT

    return translations;
}

std::unique_ptr<ComboboxTranslationMap> ComboboxEnumeration(QObject* parent) {
    std::unique_ptr<ComboboxTranslationMap> translations =
        std::make_unique<ComboboxTranslationMap>();
    const auto& tr = [](const char* text, const char* context = "") {
        return QCoreApplication::translate("ConfigurationShared", text, context);
    };

#define PAIR(ENUM, VALUE, TRANSLATION) {static_cast<u32>(Settings::ENUM::VALUE), (TRANSLATION)}

    // Intentionally skipping VSyncMode to let the UI fill that one out
    translations->insert({Settings::EnumMetadata<Settings::AppletMode>::Index(),
                          {
                              PAIR(AppletMode, HLE, tr("Custom frontend")),
                              PAIR(AppletMode, LLE, tr("Real applet")),
                              PAIR(AppletMode, Disabled, tr("Disabled")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::SpirvOptimizeMode>::Index(),
                          {
                              PAIR(SpirvOptimizeMode, Never, tr("Never")),
                              PAIR(SpirvOptimizeMode, OnLoad, tr("On Load")),
                              PAIR(SpirvOptimizeMode, Always, tr("Always")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AstcDecodeMode>::Index(),
                          {
                              PAIR(AstcDecodeMode, Cpu, tr("CPU")),
                              PAIR(AstcDecodeMode, Gpu, tr("GPU")),
                              PAIR(AstcDecodeMode, CpuAsynchronous, tr("CPU Asynchronous")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::AstcRecompression>::Index(),
         {
             PAIR(AstcRecompression, Uncompressed, tr("Uncompressed (Best quality)")),
             PAIR(AstcRecompression, Bc1, tr("BC1 (Low quality)")),
             PAIR(AstcRecompression, Bc3, tr("BC3 (Medium quality)")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::FramePacingMode>::Index(),
                          {
                              PAIR(FramePacingMode, Target_Auto, tr("Auto")),
                              PAIR(FramePacingMode, Target_30, tr("30 FPS")),
                              PAIR(FramePacingMode, Target_60, tr("60 FPS")),
                              PAIR(FramePacingMode, Target_90, tr("90 FPS")),
                              PAIR(FramePacingMode, Target_120, tr("120 FPS")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::VramUsageMode>::Index(),
                          {
                              PAIR(VramUsageMode, Conservative, tr("Conservative")),
                              PAIR(VramUsageMode, Aggressive, tr("Aggressive")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::RendererBackend>::Index(),
         {PAIR(RendererBackend, Vulkan, tr("Vulkan")),
#ifdef HAS_OPENGL
          PAIR(RendererBackend, OpenGL_GLSL, tr("OpenGL GLSL")),
          PAIR(RendererBackend, OpenGL_GLASM, tr("OpenGL GLASM (Assembly Shaders, NVIDIA Only)")),
          PAIR(RendererBackend, OpenGL_SPIRV, tr("OpenGL SPIR-V (Experimental, AMD/Mesa Only)")),
#endif
          PAIR(RendererBackend, Null, tr("Null"))}});
    translations->insert({Settings::EnumMetadata<Settings::GpuAccuracy>::Index(),
                          {
                              PAIR(GpuAccuracy, Low, tr("Fast")),
                              PAIR(GpuAccuracy, High, tr("Accurate")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::DmaAccuracy>::Index(),
                          {
                              PAIR(DmaAccuracy, Default, tr("Default")),
                              PAIR(DmaAccuracy, Unsafe, tr("Unsafe (fast)")),
                              PAIR(DmaAccuracy, Safe, tr("Safe (stable)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::NativeComputeWriteMark>::Index(),
                          {
                              PAIR(NativeComputeWriteMark, Auto, tr("Auto (shader extent)")),
                              PAIR(NativeComputeWriteMark, Full, tr("Full binding")),
                              PAIR(NativeComputeWriteMark, Cap64K, tr("64 KiB cap")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuFenceBehavior>::Index(),
                          {
                              PAIR(GpuFenceBehavior, Default, tr("Default")),
                              PAIR(GpuFenceBehavior, Immediate, tr("Immediate")),
                              PAIR(GpuFenceBehavior, Balanced, tr("Balanced")),
                              PAIR(GpuFenceBehavior, Accurate, tr("Accurate")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::CpuAccuracy>::Index(),
         {
             PAIR(CpuAccuracy, Auto, tr("Auto")),
             PAIR(CpuAccuracy, Accurate, tr("Accurate")),
             PAIR(CpuAccuracy, Unsafe, tr("Unsafe")),
             PAIR(CpuAccuracy, Paranoid, tr("Paranoid (disables most optimizations)")),
             PAIR(CpuAccuracy, Debugging, tr("Debugging")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::CpuBackend>::Index(),
                          {
                              PAIR(CpuBackend, Dynarmic, tr("Dynarmic")),
                              PAIR(CpuBackend, Nce, tr("NCE")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::FullscreenMode>::Index(),
                          {
                              PAIR(FullscreenMode, Borderless, tr("Borderless Windowed")),
                              PAIR(FullscreenMode, Exclusive, tr("Exclusive Fullscreen")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::NvdecEmulation>::Index(),
                          {
                              PAIR(NvdecEmulation, Off, tr("No Video Output")),
                              PAIR(NvdecEmulation, Cpu, tr("CPU Video Decoding")),
                              PAIR(NvdecEmulation, Gpu, tr("GPU Video Decoding (Default)")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::ResolutionSetup>::Index(),
         {
             PAIR(ResolutionSetup, Res1_4X, tr("0.25X (180p/270p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res1_2X, tr("0.5X (360p/540p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res3_4X, tr("0.75X (540p/810p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res1X, tr("1X (720p/1080p)")),
             PAIR(ResolutionSetup, Res5_4X, tr("1.25X (900p/1350p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res3_2X, tr("1.5X (1080p/1620p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res2X, tr("2X (1440p/2160p)")),
             PAIR(ResolutionSetup, Res3X, tr("3X (2160p/3240p)")),
             PAIR(ResolutionSetup, Res4X, tr("4X (2880p/4320p)")),
             PAIR(ResolutionSetup, Res5X, tr("5X (3600p/5400p)")),
             PAIR(ResolutionSetup, Res6X, tr("6X (4320p/6480p)")),
             PAIR(ResolutionSetup, Res7X, tr("7X (5040p/7560p)")),
             PAIR(ResolutionSetup, Res8X, tr("8X (5760p/8640p)")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::FrameGenType>::Index(),
                          {
                              PAIR(FrameGenType, Lsfg, tr("Lossless Scaling (LSFG)")),
                              PAIR(FrameGenType, Fsr3, tr("Optical Flow Interpolation (built-in)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::ScalingFilter>::Index(),
                          {
                              PAIR(ScalingFilter, NearestNeighbor, tr("Nearest Neighbor")),
                              PAIR(ScalingFilter, Bilinear, tr("Bilinear")),
                              PAIR(ScalingFilter, Bicubic, tr("Bicubic")),
                              PAIR(ScalingFilter, Gaussian, tr("Gaussian")),
                              PAIR(ScalingFilter, Lanczos, tr("Lanczos")),
                              PAIR(ScalingFilter, ScaleForce, tr("ScaleForce")),
                              PAIR(ScalingFilter, Fsr, tr("AMD FidelityFX Super Resolution 1 (FSR 1)")),
                              PAIR(ScalingFilter, Area, tr("Area")),
                              PAIR(ScalingFilter, Mmpx, tr("MMPX")),
                              PAIR(ScalingFilter, ZeroTangent, tr("Zero-Tangent")),
                              PAIR(ScalingFilter, BSpline, tr("B-Spline")),
                              PAIR(ScalingFilter, Mitchell, tr("Mitchell")),
                              PAIR(ScalingFilter, Spline1, tr("Spline-1")),
                              PAIR(ScalingFilter, Sgsr, tr("Snapdragon Game Super Resolution")),
                              PAIR(ScalingFilter, SgsrEdge, tr("Snapdragon Game Super Resolution EdgeDir")),
                              PAIR(ScalingFilter, Fsr3, tr("AMD FidelityFX Super Resolution 1, softer sharpening")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AntiAliasing>::Index(),
                          {
                              PAIR(AntiAliasing, None, tr("None")),
                              PAIR(AntiAliasing, Fxaa, tr("FXAA")),
                              PAIR(AntiAliasing, Smaa, tr("SMAA")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::OotAntiAliasing>::Index(),
                          {
                              PAIR(OotAntiAliasing, Game, tr("Game")),
                              PAIR(OotAntiAliasing, NoSharpening, tr("No sharpening")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AspectRatio>::Index(),
                          {
                              PAIR(AspectRatio, R16_9, tr("Default (16:9)")),
                              PAIR(AspectRatio, R4_3, tr("Force 4:3")),
                              PAIR(AspectRatio, R21_9, tr("Force 21:9")),
                              PAIR(AspectRatio, R16_10, tr("Force 16:10")),
                              PAIR(AspectRatio, Stretch, tr("Stretch to Window")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AnisotropyMode>::Index(),
                          {
                              PAIR(AnisotropyMode, Automatic, tr("Automatic")),
                              PAIR(AnisotropyMode, Default, tr("Default")),
                              PAIR(AnisotropyMode, X2, tr("2x")),
                              PAIR(AnisotropyMode, X4, tr("4x")),
                              PAIR(AnisotropyMode, X8, tr("8x")),
                              PAIR(AnisotropyMode, X16, tr("16x")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::Language>::Index(),
         {
             PAIR(Language, Japanese, tr("Japanese (日本語)")),
             PAIR(Language, EnglishAmerican, tr("American English")),
             PAIR(Language, French, tr("French (français)")),
             PAIR(Language, German, tr("German (Deutsch)")),
             PAIR(Language, Italian, tr("Italian (italiano)")),
             PAIR(Language, Spanish, tr("Spanish (español)")),
             PAIR(Language, Chinese, tr("Chinese")),
             PAIR(Language, Korean, tr("Korean (한국어)")),
             PAIR(Language, Dutch, tr("Dutch (Nederlands)")),
             PAIR(Language, Portuguese, tr("Portuguese (português)")),
             PAIR(Language, Russian, tr("Russian (Русский)")),
             PAIR(Language, Taiwanese, tr("Taiwanese")),
             PAIR(Language, EnglishBritish, tr("British English")),
             PAIR(Language, FrenchCanadian, tr("Canadian French")),
             PAIR(Language, SpanishLatin, tr("Latin American Spanish")),
             PAIR(Language, ChineseSimplified, tr("Simplified Chinese")),
             PAIR(Language, ChineseTraditional, tr("Traditional Chinese (正體中文)")),
             PAIR(Language, PortugueseBrazilian, tr("Brazilian Portuguese (português do Brasil)")),
             PAIR(Language, Polish, tr("Polish (polska)")),
             PAIR(Language, Thai, tr("Thai (แบบไทย)")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::Region>::Index(),
                          {
                              PAIR(Region, Japan, tr("Japan")),
                              PAIR(Region, Usa, tr("USA")),
                              PAIR(Region, Europe, tr("Europe")),
                              PAIR(Region, Australia, tr("Australia")),
                              PAIR(Region, China, tr("China")),
                              PAIR(Region, Korea, tr("Korea")),
                              PAIR(Region, Taiwan, tr("Taiwan")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::TimeZone>::Index(),
         {
             {static_cast<u32>(Settings::TimeZone::Auto),
              tr("Auto (%1)", "Auto select time zone")
                  .arg(QString::fromStdString(
                      Settings::GetTimeZoneString(Settings::TimeZone::Auto)))},
             {static_cast<u32>(Settings::TimeZone::Default),
              tr("Default (%1)", "Default time zone")
                  .arg(QString::fromStdString(Common::TimeZone::GetDefaultTimeZone()))},
             PAIR(TimeZone, Cet, tr("CET")),
             PAIR(TimeZone, Cst6Cdt, tr("CST6CDT")),
             PAIR(TimeZone, Cuba, tr("Cuba")),
             PAIR(TimeZone, Eet, tr("EET")),
             PAIR(TimeZone, Egypt, tr("Egypt")),
             PAIR(TimeZone, Eire, tr("Eire")),
             PAIR(TimeZone, Est, tr("EST")),
             PAIR(TimeZone, Est5Edt, tr("EST5EDT")),
             PAIR(TimeZone, Gb, tr("GB")),
             PAIR(TimeZone, GbEire, tr("GB-Eire")),
             PAIR(TimeZone, Gmt, tr("GMT")),
             PAIR(TimeZone, GmtPlusZero, tr("GMT+0")),
             PAIR(TimeZone, GmtMinusZero, tr("GMT-0")),
             PAIR(TimeZone, GmtZero, tr("GMT0")),
             PAIR(TimeZone, Greenwich, tr("Greenwich")),
             PAIR(TimeZone, Hongkong, tr("Hongkong")),
             PAIR(TimeZone, Hst, tr("HST")),
             PAIR(TimeZone, Iceland, tr("Iceland")),
             PAIR(TimeZone, Iran, tr("Iran")),
             PAIR(TimeZone, Israel, tr("Israel")),
             PAIR(TimeZone, Jamaica, tr("Jamaica")),
             PAIR(TimeZone, Japan, tr("Japan")),
             PAIR(TimeZone, Kwajalein, tr("Kwajalein")),
             PAIR(TimeZone, Libya, tr("Libya")),
             PAIR(TimeZone, Met, tr("MET")),
             PAIR(TimeZone, Mst, tr("MST")),
             PAIR(TimeZone, Mst7Mdt, tr("MST7MDT")),
             PAIR(TimeZone, Navajo, tr("Navajo")),
             PAIR(TimeZone, Nz, tr("NZ")),
             PAIR(TimeZone, NzChat, tr("NZ-CHAT")),
             PAIR(TimeZone, Poland, tr("Poland")),
             PAIR(TimeZone, Portugal, tr("Portugal")),
             PAIR(TimeZone, Prc, tr("PRC")),
             PAIR(TimeZone, Pst8Pdt, tr("PST8PDT")),
             PAIR(TimeZone, Roc, tr("ROC")),
             PAIR(TimeZone, Rok, tr("ROK")),
             PAIR(TimeZone, Singapore, tr("Singapore")),
             PAIR(TimeZone, Turkey, tr("Turkey")),
             PAIR(TimeZone, Uct, tr("UCT")),
             PAIR(TimeZone, Universal, tr("Universal")),
             PAIR(TimeZone, Utc, tr("UTC")),
             PAIR(TimeZone, WSu, tr("W-SU")),
             PAIR(TimeZone, Wet, tr("WET")),
             PAIR(TimeZone, Zulu, tr("Zulu")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::AudioMode>::Index(),
                          {
                              PAIR(AudioMode, Mono, tr("Mono")),
                              PAIR(AudioMode, Stereo, tr("Stereo")),
                              PAIR(AudioMode, Surround, tr("Surround")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::MemoryLayout>::Index(),
                          {
                              PAIR(MemoryLayout, Memory_4Gb, tr("4GB DRAM (Default)")),
                              PAIR(MemoryLayout, Memory_6Gb, tr("6GB DRAM (Unsafe)")),
                              PAIR(MemoryLayout, Memory_8Gb, tr("8GB DRAM")),
                              PAIR(MemoryLayout, Memory_10Gb, tr("10GB DRAM (Unsafe)")),
                              PAIR(MemoryLayout, Memory_12Gb, tr("12GB DRAM (Unsafe)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::ConsoleMode>::Index(),
                          {
                              PAIR(ConsoleMode, Docked, tr("Docked")),
                              PAIR(ConsoleMode, Handheld, tr("Handheld")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::NativeAsyncDraws>::Index(),
                          {
                              PAIR(NativeAsyncDraws, Wait, tr("Wait")),
                              PAIR(NativeAsyncDraws, Bounded, tr("Bounded wait")),
                              PAIR(NativeAsyncDraws, Skip, tr("Skip")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::ShaderPreloadMemory>::Index(),
                          {
                              PAIR(ShaderPreloadMemory, Off, tr("Off")),
                              PAIR(ShaderPreloadMemory, Auto, tr("Automatic")),
                              PAIR(ShaderPreloadMemory, Gib1, tr("1 GB")),
                              PAIR(ShaderPreloadMemory, Gib2, tr("2 GB")),
                              PAIR(ShaderPreloadMemory, Gib4, tr("4 GB")),
                              PAIR(ShaderPreloadMemory, Gib8, tr("8 GB")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::CpuClock>::Index(),
                          {
                              PAIR(CpuClock, Normal, tr("Normal")),
                              PAIR(CpuClock, Boost, tr("Boost")),
                              PAIR(CpuClock, Overclock, tr("Overclock")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::ConfirmStop>::Index(),
         {
             PAIR(ConfirmStop, Ask_Always, tr("Always ask (Default)")),
             PAIR(ConfirmStop, Ask_Based_On_Game, tr("Only if game specifies not to stop")),
             PAIR(ConfirmStop, Ask_Never, tr("Never ask")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::GpuClock>::Index(),
                          {
                              PAIR(GpuClock, Normal, tr("Normal")),
                              PAIR(GpuClock, Boost, tr("Boost")),
                              PAIR(GpuClock, Overclock, tr("Overclock")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuUnswizzleSize>::Index(),
                          {
                              PAIR(GpuUnswizzleSize, VerySmall, tr("Very Small (16 MB)")),
                              PAIR(GpuUnswizzleSize, Small, tr("Small (32 MB)")),
                              PAIR(GpuUnswizzleSize, Normal, tr("Normal (128 MB)")),
                              PAIR(GpuUnswizzleSize, Large, tr("Large (256 MB)")),
                              PAIR(GpuUnswizzleSize, VeryLarge, tr("Very Large (512 MB)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuUnswizzle>::Index(),
                          {
                              PAIR(GpuUnswizzle, VeryLow, tr("Very Low (4 MB)")),
                              PAIR(GpuUnswizzle, Low, tr("Low (8 MB)")),
                              PAIR(GpuUnswizzle, Normal, tr("Normal (16 MB)")),
                              PAIR(GpuUnswizzle, Medium, tr("Medium (32 MB)")),
                              PAIR(GpuUnswizzle, High, tr("High (64 MB)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuUnswizzleChunk>::Index(),
                          {
                              PAIR(GpuUnswizzleChunk, VeryLow, tr("Very Low (32)")),
                              PAIR(GpuUnswizzleChunk, Low, tr("Low (64)")),
                              PAIR(GpuUnswizzleChunk, Normal, tr("Normal (128)")),
                              PAIR(GpuUnswizzleChunk, Medium, tr("Medium (256)")),
                              PAIR(GpuUnswizzleChunk, High, tr("High (512)")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::ExtendedDynamicState>::Index(),
                          {
                              PAIR(ExtendedDynamicState, Disabled, tr("Disabled")),
                              PAIR(ExtendedDynamicState, EDS1, tr("ExtendedDynamicState 1")),
                              PAIR(ExtendedDynamicState, EDS2, tr("ExtendedDynamicState 2")),
                              PAIR(ExtendedDynamicState, EDS3, tr("ExtendedDynamicState 3")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::GameListMode>::Index(),
                          {
                              PAIR(GameListMode, TreeView, tr("Tree View")),
                              PAIR(GameListMode, GridView, tr("Grid View")),
                          }});

#undef PAIR
#undef CTX_PAIR

    return translations;
}
} // namespace ConfigurationShared

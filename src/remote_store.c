#include "remote_store.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNI_REMOTE_FILETYPE "Flipper Uni Remote"
#define UNI_REMOTE_VERSION 1U
#define UNI_REMOTES_DIR APP_DATA_PATH("remotes")
#define UNI_SEED_MARKER APP_DATA_PATH("seed_v2.done")
#define UNI_DEFAULT_DIR APP_DATA_PATH("remotes/demo_tv")
#define UNI_DEFAULT_REMOTE APP_DATA_PATH("remotes/demo_tv/remote.ur")
#define UNI_DEFAULT_SIGNALS APP_DATA_PATH("remotes/demo_tv/signals.ir")
#define UNI_DEFAULT_ACTIONS APP_DATA_PATH("remotes/demo_tv/actions.ur")

static bool write_text_file(Storage* storage, const char* path, const char* text) {
    File* file = storage_file_alloc(storage);
    if(!file) return false;

    bool ok = storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        const size_t length = strlen(text);
        ok = storage_file_write(file, text, length) == length;
        storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    return ok;
}

static void ensure_ir_package(
    Storage* storage,
    const char* folder,
    const char* remote_text,
    const char* signal_text) {
    char dir[UNI_PATH_MAX];
    char remote_path[UNI_PATH_MAX];
    char signal_path[UNI_PATH_MAX];

    snprintf(dir, sizeof(dir), UNI_REMOTES_DIR "/%.120s", folder);
    storage_common_mkdir(storage, dir);
    snprintf(remote_path, sizeof(remote_path), UNI_REMOTES_DIR "/%.120s/remote.ur", folder);
    snprintf(signal_path, sizeof(signal_path), UNI_REMOTES_DIR "/%.120s/signals.ir", folder);

    if(!storage_file_exists(storage, remote_path)) {
        write_text_file(storage, remote_path, remote_text);
    }
    if(!storage_file_exists(storage, signal_path)) {
        write_text_file(storage, signal_path, signal_text);
    }
}

static void ensure_default_package(Storage* storage) {
    storage_common_mkdir(storage, UNI_REMOTES_DIR);
    storage_common_mkdir(storage, UNI_DEFAULT_DIR);

    if(!storage_file_exists(storage, UNI_DEFAULT_REMOTE)) {
        static const char remote_text[] =
            "Filetype: Flipper Uni Remote\n"
            "Version: 1\n"
            "Id: demo_tv\n"
            "Name: Demo TV\n"
            "ShortName: TV\n"
            "Transport: IR\n"
            "Order: 10\n"
            "RepeatEnabled: true\n"
            "IrBurst: 1\n"
            "SignalFile: signals.ir\n"
            "ActionFile: actions.ur\n"
            "BluetoothProfile: tv_demo\n"
            "HardUpHold: \n"
            "HardDownHold: \n"
            "HardLeftHold: \n"
            "HardRightHold: \n"
            "HardOkHold: \n"
            "ElementCount: 5\n"
            "#\n"
            "Element0Type: status\n"
            "Element0Id: status\n"
            "Element0Rect: 0 0 3 1\n"
            "#\n"
            "Element1Type: screen\n"
            "Element1Id: main\n"
            "Element1Rect: 0 1 3 2\n"
            "Element1Label: READY\n"
            "#\n"
            "Element2Type: hstep\n"
            "Element2Id: channel\n"
            "Element2Rect: 0 3 3 1\n"
            "Element2Label: CH\n"
            "Element2Left: sig:Prev\n"
            "Element2Right: sig:Next\n"
            "#\n"
            "Element3Type: vstep\n"
            "Element3Id: volume\n"
            "Element3Rect: 0 4 1 2\n"
            "Element3Label: VOL\n"
            "Element3Up: sig:VolUp\n"
            "Element3Down: sig:VolDown\n"
            "#\n"
            "Element4Type: button\n"
            "Element4Id: power\n"
            "Element4Rect: 1 4 1 2\n"
            "Element4Label: PWR\n"
            "Element4Icon: pwr\n"
            "Element4Tap: sig:Power\n"
            "Element4Hold: sig:Mute\n";
        write_text_file(storage, UNI_DEFAULT_REMOTE, remote_text);
    }

    if(!storage_file_exists(storage, UNI_DEFAULT_SIGNALS)) {
        static const char signal_text[] =
            "Filetype: IR signals file\n"
            "Version: 1\n"
            "#\n"
            "name: Power\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 45 00 00 00\n"
            "#\n"
            "name: Mute\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 47 00 00 00\n"
            "#\n"
            "name: VolUp\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 18 00 00 00\n"
            "#\n"
            "name: VolDown\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 52 00 00 00\n"
            "#\n"
            "name: Prev\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 08 00 00 00\n"
            "#\n"
            "name: Next\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 5A 00 00 00\n";
        write_text_file(storage, UNI_DEFAULT_SIGNALS, signal_text);
    }

    if(!storage_file_exists(storage, UNI_DEFAULT_ACTIONS)) {
        static const char action_text[] =
            "Filetype: Flipper Uni Remote Actions\n"
            "Version: 1\n"
            "ActionCount: 1\n"
            "#\n"
            "Action0Id: quiet\n"
            "Action0Type: sequence\n"
            "Action0StepCount: 2\n"
            "Action0Step0: VolDown\n"
            "Action0Delay0: 120\n"
            "Action0Step1: VolDown\n"
            "Action0Delay1: 0\n";
        write_text_file(storage, UNI_DEFAULT_ACTIONS, action_text);
    }

    static const struct {
        const char* folder;
        const char* name;
        const char* short_name;
        const char* profile;
        uint32_t order;
    } ac_demos[] = {
        {"demo_lg_ac", "LG AC", "LG", "LG_AKB75215401", 20},
        {"demo_daikin_ac", "Daikin AC", "DAI", "DAIKIN_ARC433A73", 30},
        {"demo_panasonic_ac", "Panasonic AC", "PAN", "PANASONIC_RKR", 40},
        {"demo_carrier_ac", "Carrier AC", "CAR", "CARRIER_WC_UA4NE", 50},
    };

    for(size_t d = 0; d < sizeof(ac_demos) / sizeof(ac_demos[0]); d++) {
        char dir[UNI_PATH_MAX];
        char path[UNI_PATH_MAX];
        snprintf(dir, sizeof(dir), UNI_REMOTES_DIR "/%s", ac_demos[d].folder);
        storage_common_mkdir(storage, dir);
        snprintf(
            path,
            sizeof(path),
            UNI_REMOTES_DIR "/%.120s/remote.ur",
            ac_demos[d].folder);
        if(storage_file_exists(storage, path)) continue;

        char text[4096];
        const bool lg = strcmp(ac_demos[d].profile, "LG_AKB75215401") == 0;
        const bool daikin = strcmp(ac_demos[d].profile, "DAIKIN_ARC433A73") == 0;
        const bool carrier = strcmp(ac_demos[d].profile, "CARRIER_WC_UA4NE") == 0;
        const bool panasonic = strcmp(ac_demos[d].profile, "PANASONIC_RKR") == 0;
        const char* swing_action = lg ? "swingv+" : (panasonic ? "swing+" : "swing");
        snprintf(
            text,
            sizeof(text),
            "Filetype: Flipper Uni Remote\n"
            "Version: 1\n"
            "Id: %s\n"
            "Name: %s\n"
            "ShortName: %s\n"
            "Transport: STATE_IR\n"
            "StateProfile: %s\n"
            "Order: %lu\n"
            "RepeatEnabled: false\n"
            "IrBurst: 1\n"
            "PageCount: 2\n"
            "SignalFile: \n"
            "ActionFile: \n"
            "BluetoothProfile: \n"
            "HardUpHold: \n"
            "HardDownHold: \n"
            "HardLeftHold: \n"
            "HardRightHold: \n"
            "HardOkHold: \n"
            "ElementCount: %u\n"
            "#\n"
            "Element0Type: button\nElement0Id: power\nElement0Rect: 0 0 1 1\nElement0Label: PWR\nElement0Icon: pwr\nElement0Tap: st:power\n"
            "#\n"
            "Element1Type: button\nElement1Id: mode\nElement1Rect: 1 0 1 1\nElement1Label: MODE\nElement1Icon: mode\nElement1Tap: st:mode+\n"
            "#\n"
            "Element2Type: button\nElement2Id: fan\nElement2Rect: 2 0 1 1\nElement2Label: FAN\nElement2Icon: fan\nElement2Tap: st:fan+\n"
            "#\n"
            "Element3Type: button\nElement3Id: tempdn\nElement3Rect: 0 1 1 1\nElement3Label: T-\nElement3Icon: minus\nElement3Tap: st:temp-\n"
            "#\n"
            "Element4Type: button\nElement4Id: tempup\nElement4Rect: 1 1 1 1\nElement4Label: T+\nElement4Icon: plus\nElement4Tap: st:temp+\n"
            "#\n"
            "Element5Page: 1\nElement5Type: button\nElement5Id: swing\nElement5Rect: 2 1 1 1\nElement5Label: SWG\nElement5Icon: swing\nElement5Tap: st:%s\n"
            "#\n"
            "Element6Page: 1\nElement6Type: button\nElement6Id: eco\nElement6Rect: 0 2 1 1\nElement6Label: ECO\nElement6Icon: eco\nElement6Tap: st:eco\n"
            "#\n"
            "Element7Page: 1\nElement7Type: button\nElement7Id: turbo\nElement7Rect: 1 2 1 1\nElement7Label: TUR\nElement7Icon: turbo\nElement7Tap: st:turbo\n"
            "%s",
            ac_demos[d].folder,
            ac_demos[d].name,
            ac_demos[d].short_name,
            ac_demos[d].profile,
            (unsigned long)ac_demos[d].order,
            carrier ? 9U : (panasonic ? 9U : (daikin ? 8U : 8U)),
            swing_action,
            carrier ?
                "#\nElement8Page: 1\nElement8Type: button\nElement8Id: fix\nElement8Rect: 2 2 1 1\nElement8Label: FIX\nElement8Icon: down\nElement8Tap: st:fix\n" :
            panasonic ?
                "#\nElement8Page: 1\nElement8Type: button\nElement8Id: nanoe\nElement8Rect: 2 2 1 1\nElement8Label: NANO\nElement8Icon: fan\nElement8Tap: st:nanoe\n" :
                "");
        write_text_file(storage, path, text);
    }

    static const char sony_remote[] =
        "Filetype: Flipper Uni Remote\n"
        "Version: 1\n"
        "Id: sony_rm_pj8\n"
        "Name: Sony RM-PJ8\n"
        "ShortName: SON\n"
        "Transport: IR\n"
        "Order: 60\n"
        "RepeatEnabled: true\n"
        "IrBurst: 3\n"
        "PageCount: 2\n"
        "SignalFile: signals.ir\n"
        "ActionFile: \n"
        "BluetoothProfile: \n"
        "HardUpHold: \nHardDownHold: \nHardLeftHold: \nHardRightHold: \nHardOkHold: \n"
        "ElementCount: 18\n"
        "#\nElement0Type: button\nElement0Id: power\nElement0Rect: 0 0 1 1\nElement0Label: PWR\nElement0Icon: pwr\nElement0Tap: sig:Power\n"
        "#\nElement1Type: button\nElement1Id: input\nElement1Rect: 1 0 1 1\nElement1Label: IN\nElement1Icon: input\nElement1Tap: sig:Input\n"
        "#\nElement2Type: button\nElement2Id: menu\nElement2Rect: 2 0 1 1\nElement2Label: MENU\nElement2Icon: menu\nElement2Tap: sig:Menu\n"
        "#\nElement3Type: dpad\nElement3Id: nav\nElement3Rect: 0 1 3 3\nElement3Up: sig:Up\nElement3Down: sig:Down\nElement3Left: sig:Left\nElement3Right: sig:Right\nElement3Ok: sig:Enter\n"
        "#\nElement4Type: button\nElement4Id: return\nElement4Rect: 0 1 1 1\nElement4Label: RET\nElement4Icon: back\nElement4Tap: sig:Return\n"
        "#\nElement5Type: button\nElement5Id: aspect\nElement5Rect: 2 1 1 1\nElement5Label: ASP\nElement5Icon: asp\nElement5Tap: sig:Aspect\n"
        "#\nElement6Type: button\nElement6Id: voldn\nElement6Rect: 0 3 1 1\nElement6Label: V-\nElement6Icon: volm\nElement6Tap: sig:Vol_dn\n"
        "#\nElement7Type: button\nElement7Id: volup\nElement7Rect: 2 3 1 1\nElement7Label: V+\nElement7Icon: volp\nElement7Tap: sig:Vol_up\n"
        "#\nElement8Type: button\nElement8Id: freeze\nElement8Rect: 0 4 1 1\nElement8Label: FRZ\nElement8Icon: frz\nElement8Tap: sig:Freeze\n"
        "#\nElement9Type: button\nElement9Id: blank\nElement9Rect: 1 4 1 1\nElement9Label: BLK\nElement9Icon: blank\nElement9Tap: sig:Blank\n"
        "#\nElement10Type: button\nElement10Id: mute\nElement10Rect: 2 4 1 1\nElement10Label: MUT\nElement10Icon: mut\nElement10Tap: sig:Muting\n"
        "#\nElement11Page: 1\nElement11Type: button\nElement11Id: apa\nElement11Rect: 0 0 1 1\nElement11Label: APA\nElement11Icon: auto\nElement11Tap: sig:Apa\n"
        "#\nElement12Page: 1\nElement12Type: button\nElement12Id: eco\nElement12Rect: 1 0 1 1\nElement12Label: ECO\nElement12Icon: eco\nElement12Tap: sig:Eco_mode\n"
        "#\nElement13Page: 1\nElement13Type: button\nElement13Id: reset\nElement13Rect: 2 0 1 1\nElement13Label: RST\nElement13Icon: back\nElement13Tap: sig:Reset\n"
        "#\nElement14Page: 1\nElement14Type: button\nElement14Id: key\nElement14Rect: 0 1 1 1\nElement14Label: KEY\nElement14Icon: key\nElement14Tap: sig:Keystone\n"
        "#\nElement15Page: 1\nElement15Type: button\nElement15Id: pattern\nElement15Rect: 1 1 1 1\nElement15Label: PAT\nElement15Tap: sig:Pattern\n"
        "#\nElement16Page: 1\nElement16Type: button\nElement16Id: zoomdn\nElement16Rect: 0 2 1 1\nElement16Label: Z-\nElement16Icon: zoom\nElement16Tap: sig:D_zoom_down\n"
        "#\nElement17Page: 1\nElement17Type: button\nElement17Id: zoomup\nElement17Rect: 1 2 1 1\nElement17Label: Z+\nElement17Icon: zoom\nElement17Tap: sig:D_zoom_up\n";

    static const char sony_signals[] =
        "Filetype: IR signals file\nVersion: 1\n"
        "#\nname: Power\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 15 00 00 00\n"
        "#\nname: Input\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 57 00 00 00\n"
        "#\nname: Apa\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 60 00 00 00\n"
        "#\nname: Eco_mode\ntype: parsed\nprotocol: SIRC20\naddress: FA 04 00 00\ncommand: 11 00 00 00\n"
        "#\nname: Menu\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 29 00 00 00\n"
        "#\nname: Reset\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 7B 00 00 00\n"
        "#\nname: Up\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 35 00 00 00\n"
        "#\nname: Down\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 36 00 00 00\n"
        "#\nname: Left\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 34 00 00 00\n"
        "#\nname: Right\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 33 00 00 00\n"
        "#\nname: Enter\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 5A 00 00 00\n"
        "#\nname: Return\ntype: parsed\nprotocol: SIRC20\naddress: FA 04 00 00\ncommand: 6F 00 00 00\n"
        "#\nname: Aspect\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 6E 00 00 00\n"
        "#\nname: Keystone\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 3A 00 00 00\n"
        "#\nname: Pattern\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 7E 00 00 00\n"
        "#\nname: D_zoom_up\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 6A 00 00 00\n"
        "#\nname: D_zoom_down\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 6B 00 00 00\n"
        "#\nname: Vol_up\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 12 00 00 00\n"
        "#\nname: Vol_dn\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 13 00 00 00\n"
        "#\nname: Freeze\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 67 00 00 00\n"
        "#\nname: Blank\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 24 00 00 00\n"
        "#\nname: Muting\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 14 00 00 00\n";

    static const char optoma_remote[] =
        "Filetype: Flipper Uni Remote\n"
        "Version: 1\n"
        "Id: optoma_hr21g\n"
        "Name: Optoma HR21G\n"
        "ShortName: OPT\n"
        "Transport: IR\n"
        "Order: 70\n"
        "RepeatEnabled: true\n"
        "IrBurst: 1\n"
        "PageCount: 2\n"
        "SignalFile: signals.ir\n"
        "ActionFile: \n"
        "BluetoothProfile: \n"
        "HardUpHold: \nHardDownHold: \nHardLeftHold: \nHardRightHold: \nHardOkHold: \n"
        "ElementCount: 15\n"
        "#\nElement0Type: button\nElement0Id: power\nElement0Rect: 0 0 1 1\nElement0Label: PWR\nElement0Icon: pwr\nElement0Tap: sig:Power\n"
        "#\nElement1Type: button\nElement1Id: source\nElement1Rect: 1 0 1 1\nElement1Label: SRC\nElement1Icon: src\nElement1Tap: sig:Source\n"
        "#\nElement2Type: button\nElement2Id: menu\nElement2Rect: 2 0 1 1\nElement2Label: MENU\nElement2Icon: menu\nElement2Tap: sig:Menu\n"
        "#\nElement3Type: dpad\nElement3Id: nav\nElement3Rect: 0 1 3 3\nElement3Up: sig:Up\nElement3Down: sig:Down\nElement3Left: sig:Left\nElement3Right: sig:Right\nElement3Ok: sig:Enter\n"
        "#\nElement4Type: button\nElement4Id: return\nElement4Rect: 0 1 1 1\nElement4Label: RET\nElement4Icon: back\nElement4Tap: sig:Return\n"
        "#\nElement5Type: button\nElement5Id: aspect\nElement5Rect: 2 1 1 1\nElement5Label: ASP\nElement5Icon: asp\nElement5Tap: sig:Aspect_ratio\n"
        "#\nElement6Type: button\nElement6Id: voldn\nElement6Rect: 0 3 1 1\nElement6Label: V-\nElement6Icon: volm\nElement6Tap: sig:Volume_down\n"
        "#\nElement7Type: button\nElement7Id: volup\nElement7Rect: 2 3 1 1\nElement7Label: V+\nElement7Icon: volp\nElement7Tap: sig:Volume_up\n"
        "#\nElement8Type: button\nElement8Id: mute\nElement8Rect: 0 4 1 1\nElement8Label: MUT\nElement8Icon: mut\nElement8Tap: sig:Mute\n"
        "#\nElement9Type: button\nElement9Id: freeze\nElement9Rect: 1 4 1 1\nElement9Label: FRZ\nElement9Icon: frz\nElement9Tap: sig:Freeze\n"
        "#\nElement10Type: button\nElement10Id: key\nElement10Rect: 2 4 1 1\nElement10Label: KEY\nElement10Icon: key\nElement10Tap: sig:Keystone\n"
        "#\nElement11Page: 1\nElement11Type: button\nElement11Id: mode\nElement11Rect: 0 0 1 1\nElement11Label: MODE\nElement11Icon: mode\nElement11Tap: sig:Mode\n"
        "#\nElement12Page: 1\nElement12Type: button\nElement12Id: settings\nElement12Rect: 1 0 1 1\nElement12Label: SET\nElement12Icon: set\nElement12Tap: sig:Settings\n"
        "#\nElement13Page: 1\nElement13Type: button\nElement13Id: avmute\nElement13Rect: 2 0 1 1\nElement13Label: AV\nElement13Icon: blank\nElement13Tap: sig:Av_mute\n"
        "#\nElement14Page: 1\nElement14Type: button\nElement14Id: source2\nElement14Rect: 1 1 1 1\nElement14Label: SRC\nElement14Icon: src\nElement14Tap: sig:Source\n";

    static const char optoma_signals[] =
        "Filetype: IR signals file\nVersion: 1\n"
        "#\nname: Power\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 02 00 00 00\n"
        "#\nname: Aspect_ratio\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 64 00 00 00\n"
        "#\nname: Source\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: C3 00 00 00\n"
        "#\nname: Mode\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 05 00 00 00\n"
        "#\nname: Up\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 11 00 00 00\n"
        "#\nname: Left\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 10 00 00 00\n"
        "#\nname: Right\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 12 00 00 00\n"
        "#\nname: Down\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 14 00 00 00\n"
        "#\nname: Enter\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 0F 00 00 00\n"
        "#\nname: Settings\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: A8 00 00 00\n"
        "#\nname: Return\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 0D 00 00 00\n"
        "#\nname: Menu\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 0E 00 00 00\n"
        "#\nname: Volume_down\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 8F 00 00 00\n"
        "#\nname: Mute\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 52 00 00 00\n"
        "#\nname: Volume_up\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 8C 00 00 00\n"
        "#\nname: Freeze\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 06 00 00 00\n"
        "#\nname: Keystone\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 07 00 00 00\n"
        "#\nname: Av_mute\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 03 00 00 00\n";

    ensure_ir_package(storage, "sony_rm_pj8", sony_remote, sony_signals);
    ensure_ir_package(storage, "optoma_hr21g", optoma_remote, optoma_signals);

    static const char bt_tv_remote[] =
        "Filetype: Flipper Uni Remote\n"
        "Version: 1\n"
        "Id: bt_smart_tv\n"
        "Name: Bluetooth TV\n"
        "ShortName: BTV\n"
        "Transport: BT\n"
        "Order: 80\n"
        "RepeatEnabled: true\n"
        "IrBurst: 1\n"
        "PageCount: 1\n"
        "SignalFile: \n"
        "ActionFile: \n"
        "BluetoothProfile: smart_tv\n"
        "HardUpHold: \nHardDownHold: \nHardLeftHold: \nHardRightHold: \nHardOkHold: \n"
        "ElementCount: 11\n"
        "#\nElement0Type: button\nElement0Id: power\nElement0Rect: 0 0 1 1\nElement0Label: PWR\nElement0Icon: pwr\nElement0Tap: bt:power\n"
        "#\nElement1Type: button\nElement1Id: home\nElement1Rect: 1 0 1 1\nElement1Label: HOME\nElement1Icon: home\nElement1Tap: bt:home\n"
        "#\nElement2Type: button\nElement2Id: back\nElement2Rect: 2 0 1 1\nElement2Label: BACK\nElement2Icon: back\nElement2Tap: bt:back\n"
        "#\nElement3Type: dpad\nElement3Id: nav\nElement3Rect: 0 1 3 3\nElement3Up: bt:up\nElement3Down: bt:down\nElement3Left: bt:left\nElement3Right: bt:right\nElement3Ok: bt:ok\n"
        "#\nElement4Type: button\nElement4Id: voldn\nElement4Rect: 0 1 1 1\nElement4Label: V-\nElement4Icon: volm\nElement4Tap: bt:vol-\n"
        "#\nElement5Type: button\nElement5Id: volup\nElement5Rect: 2 1 1 1\nElement5Label: V+\nElement5Icon: volp\nElement5Tap: bt:vol+\n"
        "#\nElement6Type: button\nElement6Id: prev\nElement6Rect: 0 3 1 1\nElement6Label: PREV\nElement6Icon: prev\nElement6Tap: bt:prev\n"
        "#\nElement7Type: button\nElement7Id: next\nElement7Rect: 2 3 1 1\nElement7Label: NEXT\nElement7Icon: next\nElement7Tap: bt:next\n"
        "#\nElement8Type: button\nElement8Id: mute\nElement8Rect: 0 4 1 1\nElement8Label: MUTE\nElement8Icon: mut\nElement8Tap: bt:mute\n"
        "#\nElement9Type: button\nElement9Id: play\nElement9Rect: 1 4 1 1\nElement9Label: PLAY\nElement9Icon: play\nElement9Tap: bt:play\n"
        "#\nElement10Type: button\nElement10Id: stop\nElement10Rect: 2 4 1 1\nElement10Label: STOP\nElement10Icon: stop\nElement10Tap: bt:stop\n";

    char bt_dir[UNI_PATH_MAX];
    char bt_path[UNI_PATH_MAX];
    snprintf(bt_dir, sizeof(bt_dir), UNI_REMOTES_DIR "/bt_smart_tv");
    storage_common_mkdir(storage, bt_dir);
    snprintf(bt_path, sizeof(bt_path), UNI_REMOTES_DIR "/bt_smart_tv/remote.ur");
    if(!storage_file_exists(storage, bt_path)) {
        write_text_file(storage, bt_path, bt_tv_remote);
    }
}


static bool ff_read_string(
    FlipperFormat* ff,
    const char* key,
    char* out,
    size_t out_size,
    bool required) {
    FuriString* value = furi_string_alloc();
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_string(ff, key, value);
    if(found) snprintf(out, out_size, "%s", furi_string_get_cstr(value));
    furi_string_free(value);
    return found || !required;
}

static bool ff_read_u32(FlipperFormat* ff, const char* key, uint32_t* value, bool required) {
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_uint32(ff, key, value, 1);
    return found || !required;
}

static bool ff_read_bool(FlipperFormat* ff, const char* key, bool* value, bool required) {
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_bool(ff, key, value, 1);
    return found || !required;
}

static bool ff_read_rect(FlipperFormat* ff, const char* key, UniElement* element) {
    uint32_t rect[4] = {0};
    flipper_format_rewind(ff);
    if(!flipper_format_read_uint32(ff, key, rect, 4)) return false;
    if(rect[0] >= 3 || rect[1] >= 6 || rect[2] == 0 || rect[3] == 0) return false;
    if(rect[0] + rect[2] > 3 || rect[1] + rect[3] > 6) return false;

    element->x = (uint8_t)rect[0];
    element->y = (uint8_t)rect[1];
    element->w = (uint8_t)rect[2];
    element->h = (uint8_t)rect[3];
    return true;
}

static bool parse_transport(const char* text, UniTransport* transport) {
    if(strcmp(text, "IR") == 0) {
        *transport = UniTransportInfrared;
        return true;
    }
    if(strcmp(text, "BT") == 0 || strcmp(text, "BLE_HID") == 0) {
        *transport = UniTransportBluetoothHid;
        return true;
    }
    if(strcmp(text, "STATE_IR") == 0) {
        *transport = UniTransportStatefulIr;
        return true;
    }
    return false;
}

static bool parse_element_type(const char* text, UniElementType* type) {
    if(strcmp(text, "status") == 0) *type = UniElementStatus;
    else if(strcmp(text, "screen") == 0) *type = UniElementScreen;
    else if(strcmp(text, "button") == 0) *type = UniElementButton;
    else if(strcmp(text, "hstep") == 0) *type = UniElementHStep;
    else if(strcmp(text, "vstep") == 0) *type = UniElementVStep;
    else if(strcmp(text, "dpad") == 0) *type = UniElementDpad;
    else return false;
    return true;
}

static void read_element_string(
    FlipperFormat* ff,
    uint32_t index,
    const char* suffix,
    char* out,
    size_t out_size) {
    char key[40];
    snprintf(key, sizeof(key), "Element%lu%s", (unsigned long)index, suffix);
    ff_read_string(ff, key, out, out_size, false);
}

static bool load_element(FlipperFormat* ff, uint32_t index, UniElement* element) {
    char key[40];
    char type_text[16] = {0};
    memset(element, 0, sizeof(UniElement));

    snprintf(key, sizeof(key), "Element%luType", (unsigned long)index);
    if(!ff_read_string(ff, key, type_text, sizeof(type_text), true)) return false;
    if(!parse_element_type(type_text, &element->type)) return false;

    snprintf(key, sizeof(key), "Element%luId", (unsigned long)index);
    if(!ff_read_string(ff, key, element->id, sizeof(element->id), true)) return false;
    uint32_t page = 0;
    snprintf(key, sizeof(key), "Element%luPage", (unsigned long)index);
    ff_read_u32(ff, key, &page, false);
    if(page >= UNI_MAX_PAGES) return false;
    element->page = (uint8_t)page;
    snprintf(key, sizeof(key), "Element%luRect", (unsigned long)index);
    if(!ff_read_rect(ff, key, element)) return false;

    read_element_string(ff, index, "Label", element->label, sizeof(element->label));
    read_element_string(ff, index, "Icon", element->icon, sizeof(element->icon));
    read_element_string(ff, index, "HoldIcon", element->hold_icon, sizeof(element->hold_icon));

    read_element_string(ff, index, "Tap", element->tap, sizeof(element->tap));
    read_element_string(ff, index, "Hold", element->hold, sizeof(element->hold));
    read_element_string(ff, index, "Up", element->up, sizeof(element->up));
    read_element_string(ff, index, "Down", element->down, sizeof(element->down));
    read_element_string(ff, index, "Left", element->left, sizeof(element->left));
    read_element_string(ff, index, "Right", element->right, sizeof(element->right));
    read_element_string(ff, index, "Ok", element->ok, sizeof(element->ok));
    read_element_string(ff, index, "UpHold", element->up_hold, sizeof(element->up_hold));
    read_element_string(ff, index, "DownHold", element->down_hold, sizeof(element->down_hold));
    read_element_string(ff, index, "LeftHold", element->left_hold, sizeof(element->left_hold));
    read_element_string(ff, index, "RightHold", element->right_hold, sizeof(element->right_hold));
    read_element_string(ff, index, "OkHold", element->ok_hold, sizeof(element->ok_hold));

    read_element_string(ff, index, "UpHoldIcon", element->up_hold_icon, sizeof(element->up_hold_icon));
    read_element_string(ff, index, "DownHoldIcon", element->down_hold_icon, sizeof(element->down_hold_icon));
    read_element_string(ff, index, "LeftHoldIcon", element->left_hold_icon, sizeof(element->left_hold_icon));
    read_element_string(ff, index, "RightHoldIcon", element->right_hold_icon, sizeof(element->right_hold_icon));
    read_element_string(ff, index, "OkHoldIcon", element->ok_hold_icon, sizeof(element->ok_hold_icon));

    snprintf(key, sizeof(key), "Element%luAltSticky", (unsigned long)index);
    ff_read_bool(ff, key, &element->alt_sticky, false);
    return true;
}

static bool load_remote(Storage* storage, const char* folder, UniRemote* remote) {
    char config_path[UNI_PATH_MAX];
    snprintf(config_path, sizeof(config_path), UNI_REMOTES_DIR "/%s/remote.ur", folder);
    if(!storage_file_exists(storage, config_path)) return false;

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    if(!ff) return false;

    bool ok = false;
    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    char transport_text[16] = {0};
    uint32_t element_count = 0;

    memset(remote, 0, sizeof(UniRemote));
    remote->repeat_enabled = true;
    remote->page_count = 1;
    remote->ir_burst_count = 1;
    snprintf(remote->id, sizeof(remote->id), "%.23s", folder);
    snprintf(remote->signal_file, sizeof(remote->signal_file), "signals.ir");
    snprintf(remote->action_file, sizeof(remote->action_file), "actions.ur");

    do {
        if(!flipper_format_file_open_existing(ff, config_path)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_REMOTE_FILETYPE) != 0) break;
        if(version != UNI_REMOTE_VERSION) break;

        ff_read_string(ff, "Id", remote->id, sizeof(remote->id), false);
        if(!ff_read_string(ff, "Name", remote->name, sizeof(remote->name), true)) break;
        if(!ff_read_string(ff, "ShortName", remote->short_name, sizeof(remote->short_name), true))
            break;
        if(!ff_read_string(ff, "Transport", transport_text, sizeof(transport_text), true)) break;
        if(!parse_transport(transport_text, &remote->transport)) break;
        ff_read_u32(ff, "Order", &remote->order, false);
        ff_read_bool(ff, "RepeatEnabled", &remote->repeat_enabled, false);
        uint32_t ir_burst = 1;
        ff_read_u32(ff, "IrBurst", &ir_burst, false);
        if(ir_burst < 1 || ir_burst > 8) break;
        remote->ir_burst_count = (uint8_t)ir_burst;
        uint32_t page_count = 1;
        ff_read_u32(ff, "PageCount", &page_count, false);
        if(page_count < 1 || page_count > UNI_MAX_PAGES) break;
        remote->page_count = (uint8_t)page_count;
        ff_read_string(ff, "SignalFile", remote->signal_file, sizeof(remote->signal_file), false);
        ff_read_string(ff, "ActionFile", remote->action_file, sizeof(remote->action_file), false);
        ff_read_string(
            ff,
            "BluetoothProfile",
            remote->bluetooth_profile,
            sizeof(remote->bluetooth_profile),
            false);
        ff_read_string(
            ff,
            "StateProfile",
            remote->state_profile,
            sizeof(remote->state_profile),
            false);

        ff_read_string(ff, "HardUpHold", remote->hard_bindings[UniHardUpHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardDownHold", remote->hard_bindings[UniHardDownHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardLeftHold", remote->hard_bindings[UniHardLeftHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardRightHold", remote->hard_bindings[UniHardRightHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardOkHold", remote->hard_bindings[UniHardOkHold], UNI_BINDING_MAX, false);

        if(!ff_read_u32(ff, "ElementCount", &element_count, true)) break;
        if(element_count > UNI_MAX_ELEMENTS) break;
        remote->element_count = element_count;
        remote->elements = NULL;
        remote->elements_loaded = false;

        char signal_file_copy[64];
        char action_file_copy[64];
        snprintf(signal_file_copy, sizeof(signal_file_copy), "%s", remote->signal_file);
        snprintf(action_file_copy, sizeof(action_file_copy), "%s", remote->action_file);

        snprintf(remote->config_path, sizeof(remote->config_path), "%s", config_path);
        snprintf(
            remote->signal_path,
            sizeof(remote->signal_path),
            UNI_REMOTES_DIR "/%s/%s",
            folder,
            signal_file_copy);
        snprintf(
            remote->action_path,
            sizeof(remote->action_path),
            UNI_REMOTES_DIR "/%s/%s",
            folder,
            action_file_copy);
        snprintf(
            remote->state_path,
            sizeof(remote->state_path),
            UNI_REMOTES_DIR "/%s/state.bin",
            folder);
        ok = true;
    } while(false);

    flipper_format_file_close(ff);
    furi_string_free(filetype);
    flipper_format_free(ff);
    return ok;
}

static void sort_remotes(UniRemoteStore* store) {
    for(size_t i = 0; i < store->count; i++) {
        for(size_t j = i + 1; j < store->count; j++) {
            if(store->remotes[j].order < store->remotes[i].order) {
                UniRemote tmp = store->remotes[i];
                store->remotes[i] = store->remotes[j];
                store->remotes[j] = tmp;
            }
        }
    }
}

static void scan_remotes(UniRemoteStore* store) {
    for(size_t i = 0; i < UNI_MAX_REMOTES; i++) {
        if(store->remotes[i].elements) {
            free(store->remotes[i].elements);
            store->remotes[i].elements = NULL;
            store->remotes[i].elements_loaded = false;
        }
    }
    store->count = 0;
    File* dir = storage_file_alloc(store->storage);
    if(!dir) return;

    if(storage_dir_open(dir, UNI_REMOTES_DIR)) {
        FileInfo info;
        char name[64];
        while(store->count < UNI_MAX_REMOTES &&
              storage_dir_read(dir, &info, name, sizeof(name))) {
            if(!(info.flags & FSF_DIRECTORY)) continue;
            if(load_remote(store->storage, name, &store->remotes[store->count])) {
                store->count++;
            }
        }
    }

    storage_dir_close(dir);
    storage_file_free(dir);
    sort_remotes(store);
}

static bool write_string(FlipperFormat* ff, const char* key, const char* value) {
    return flipper_format_write_string_cstr(ff, key, value ? value : "");
}

static bool write_element(FlipperFormat* ff, size_t index, const UniElement* e) {
    char key[40];
    uint32_t rect[4] = {e->x, e->y, e->w, e->h};

#define WRITE_STR(SUFFIX, VALUE) \
    do { \
        snprintf(key, sizeof(key), "Element%lu" SUFFIX, (unsigned long)index); \
        if(!write_string(ff, key, VALUE)) return false; \
    } while(0)

    WRITE_STR("Type", uni_element_type_name(e->type));
    WRITE_STR("Id", e->id);
    uint32_t page = e->page;
    snprintf(key, sizeof(key), "Element%luPage", (unsigned long)index);
    if(!flipper_format_write_uint32(ff, key, &page, 1)) return false;
    snprintf(key, sizeof(key), "Element%luRect", (unsigned long)index);
    if(!flipper_format_write_uint32(ff, key, rect, 4)) return false;
    WRITE_STR("Label", e->label);
    WRITE_STR("Icon", e->icon);
    WRITE_STR("HoldIcon", e->hold_icon);
    WRITE_STR("Tap", e->tap);
    WRITE_STR("Hold", e->hold);
    WRITE_STR("Up", e->up);
    WRITE_STR("Down", e->down);
    WRITE_STR("Left", e->left);
    WRITE_STR("Right", e->right);
    WRITE_STR("Ok", e->ok);
    WRITE_STR("UpHold", e->up_hold);
    WRITE_STR("DownHold", e->down_hold);
    WRITE_STR("LeftHold", e->left_hold);
    WRITE_STR("RightHold", e->right_hold);
    WRITE_STR("OkHold", e->ok_hold);
    WRITE_STR("UpHoldIcon", e->up_hold_icon);
    WRITE_STR("DownHoldIcon", e->down_hold_icon);
    WRITE_STR("LeftHoldIcon", e->left_hold_icon);
    WRITE_STR("RightHoldIcon", e->right_hold_icon);
    WRITE_STR("OkHoldIcon", e->ok_hold_icon);
    snprintf(key, sizeof(key), "Element%luAltSticky", (unsigned long)index);
    if(!flipper_format_write_bool(ff, key, &e->alt_sticky, 1)) return false;

#undef WRITE_STR
    return true;
}

bool uni_remote_store_save(UniRemoteStore* store, size_t remote_index) {
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!store || !remote || !remote->config_path[0] || !remote->elements_loaded ||
       !remote->elements) {
        return false;
    }

    FlipperFormat* ff = flipper_format_file_alloc(store->storage);
    if(!ff) return false;

    bool ok = false;
    do {
        if(!flipper_format_file_open_always(ff, remote->config_path)) break;
        if(!flipper_format_write_header_cstr(ff, UNI_REMOTE_FILETYPE, UNI_REMOTE_VERSION)) break;
        if(!write_string(ff, "Id", remote->id)) break;
        if(!write_string(ff, "Name", remote->name)) break;
        if(!write_string(ff, "ShortName", remote->short_name)) break;

        const char* transport = remote->transport == UniTransportInfrared ?
                                    "IR" :
                                remote->transport == UniTransportBluetoothHid ?
                                    "BT" :
                                    "STATE_IR";
        if(!write_string(ff, "Transport", transport)) break;
        if(!flipper_format_write_uint32(ff, "Order", &remote->order, 1)) break;
        if(!flipper_format_write_bool(ff, "RepeatEnabled", &remote->repeat_enabled, 1)) break;
        uint32_t ir_burst = remote->ir_burst_count ? remote->ir_burst_count : 1;
        if(!flipper_format_write_uint32(ff, "IrBurst", &ir_burst, 1)) break;
        uint32_t page_count = remote->page_count ? remote->page_count : 1;
        if(!flipper_format_write_uint32(ff, "PageCount", &page_count, 1)) break;
        if(!write_string(ff, "SignalFile", remote->signal_file)) break;
        if(!write_string(ff, "ActionFile", remote->action_file)) break;
        if(!write_string(ff, "BluetoothProfile", remote->bluetooth_profile)) break;
        if(!write_string(ff, "StateProfile", remote->state_profile)) break;

        if(!write_string(ff, "HardUpHold", remote->hard_bindings[UniHardUpHold])) break;
        if(!write_string(ff, "HardDownHold", remote->hard_bindings[UniHardDownHold])) break;
        if(!write_string(ff, "HardLeftHold", remote->hard_bindings[UniHardLeftHold])) break;
        if(!write_string(ff, "HardRightHold", remote->hard_bindings[UniHardRightHold])) break;
        if(!write_string(ff, "HardOkHold", remote->hard_bindings[UniHardOkHold])) break;

        uint32_t count = remote->element_count;
        if(!flipper_format_write_uint32(ff, "ElementCount", &count, 1)) break;
        for(size_t i = 0; i < remote->element_count; i++) {
            if(!write_element(ff, i, &remote->elements[i])) goto done;
        }
        ok = true;
    } while(false);

done:
    flipper_format_file_close(ff);
    flipper_format_free(ff);
    return ok;
}


void uni_remote_store_unload_details(UniRemoteStore* store, size_t remote_index) {
    if(!store || remote_index >= store->count) return;
    UniRemote* remote = &store->remotes[remote_index];
    if(remote->elements) {
        free(remote->elements);
        remote->elements = NULL;
    }
    remote->elements_loaded = false;
}

bool uni_remote_store_load_details(UniRemoteStore* store, size_t remote_index) {
    if(!store || remote_index >= store->count) return false;

    UniRemote* remote = &store->remotes[remote_index];
    if(remote->elements_loaded && remote->elements) return true;

    /* Keep at most one full remote layout resident in RAM. */
    for(size_t i = 0; i < store->count; i++) {
        if(i != remote_index) uni_remote_store_unload_details(store, i);
    }

    UniElement* elements = calloc(UNI_MAX_ELEMENTS, sizeof(UniElement));
    if(!elements) return false;

    FlipperFormat* ff = flipper_format_file_alloc(store->storage);
    if(!ff) {
        free(elements);
        return false;
    }

    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    uint32_t element_count = 0;
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(ff, remote->config_path)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_REMOTE_FILETYPE) != 0) break;
        if(version != UNI_REMOTE_VERSION) break;
        if(!ff_read_u32(ff, "ElementCount", &element_count, true)) break;
        if(element_count > UNI_MAX_ELEMENTS) break;

        for(uint32_t i = 0; i < element_count; i++) {
            if(!load_element(ff, i, &elements[i])) goto done_details;
        }

        remote->elements = elements;
        remote->element_count = element_count;
        remote->elements_loaded = true;
        elements = NULL;
        ok = true;
    } while(false);

done_details:
    flipper_format_file_close(ff);
    furi_string_free(filetype);
    flipper_format_free(ff);
    if(elements) free(elements);
    return ok;
}

bool uni_remote_store_init(UniRemoteStore* store, Storage* storage) {
    if(!store || !storage) return false;
    memset(store, 0, sizeof(UniRemoteStore));
    store->storage = storage;

    storage_common_mkdir(storage, UNI_REMOTES_DIR);

    /*
     * Seed examples once per data schema. This makes upgrades discover the new
     * examples without resurrecting a built-in remote that the user deleted later.
     */
    if(!storage_file_exists(storage, UNI_SEED_MARKER)) {
        ensure_default_package(storage);
        static const char marker[] = "v2\n";
        write_text_file(storage, UNI_SEED_MARKER, marker);
    }

    scan_remotes(store);
    if(store->count == 0) {
        /* Keep a recoverable first-run path even if the marker was copied alone. */
        ensure_default_package(storage);
        scan_remotes(store);
    }
    return store->count > 0;
}

bool uni_remote_store_reload(UniRemoteStore* store) {
    if(!store || !store->storage) return false;
    scan_remotes(store);
    return store->count > 0;
}

size_t uni_remote_store_count(const UniRemoteStore* store) {
    return store ? store->count : 0;
}

const UniRemote* uni_remote_store_get(const UniRemoteStore* store, size_t index) {
    if(!store || index >= store->count) return NULL;
    return &store->remotes[index];
}

UniRemote* uni_remote_store_get_mut(UniRemoteStore* store, size_t index) {
    if(!store || index >= store->count) return NULL;
    return &store->remotes[index];
}

size_t uni_remote_store_find_id(const UniRemoteStore* store, const char* id) {
    if(!store || !id) return 0;
    for(size_t i = 0; i < store->count; i++) {
        if(strcmp(store->remotes[i].id, id) == 0) return i;
    }
    return 0;
}

bool uni_remote_store_set_repeat(UniRemoteStore* store, size_t remote_index, bool enabled) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote) return false;
    remote->repeat_enabled = enabled;
    return uni_remote_store_save(store, remote_index);
}

static bool elements_overlap_at(
    const UniElement* a,
    uint8_t ax,
    uint8_t ay,
    const UniElement* b,
    uint8_t bx,
    uint8_t by) {
    if(!a || !b) return false;
    if(a->page != b->page) return false;
    for(uint8_t y = 0; y < 6; y++) {
        for(uint8_t x = 0; x < 3; x++) {
            if(uni_element_occupies_cell_at(a, ax, ay, x, y) &&
               uni_element_occupies_cell_at(b, bx, by, x, y)) {
                return true;
            }
        }
    }
    return false;
}

static bool element_free(
    const UniRemote* remote,
    size_t ignore,
    const UniElement* candidate,
    uint8_t x,
    uint8_t y) {
    if(!remote || !candidate) return false;
    for(size_t i = 0; i < remote->element_count; i++) {
        if(i == ignore) continue;
        const UniElement* e = &remote->elements[i];
        if(elements_overlap_at(candidate, x, y, e, e->x, e->y)) return false;
    }
    return true;
}

static bool find_nearest_free_position(
    const UniRemote* remote,
    size_t ignore,
    const UniElement* candidate,
    uint8_t preferred_x,
    uint8_t preferred_y,
    uint8_t* out_x,
    uint8_t* out_y) {
    if(!candidate) return false;
    bool found = false;
    uint16_t best_score = UINT16_MAX;

    for(uint8_t y = 0; y + candidate->h <= 6; y++) {
        for(uint8_t x = 0; x + candidate->w <= 3; x++) {
            if(!element_free(remote, ignore, candidate, x, y)) continue;
            const uint16_t dx = x > preferred_x ? x - preferred_x : preferred_x - x;
            const uint16_t dy = y > preferred_y ? y - preferred_y : preferred_y - y;
            const uint16_t score = (uint16_t)(dx + dy);
            if(!found || score < best_score) {
                found = true;
                best_score = score;
                *out_x = x;
                *out_y = y;
            }
        }
    }
    return found;
}

static void preferred_position(
    const UniElementPreset* preset,
    uint8_t* x,
    uint8_t* y) {
    *x = 0;
    *y = 0;
    if(!preset) return;

    switch(preset->type) {
    case UniElementStatus:
        *x = 0;
        *y = 0;
        break;
    case UniElementScreen:
        *x = 0;
        *y = 1;
        break;
    case UniElementDpad:
        *x = 0;
        *y = 3;
        break;
    case UniElementHStep:
        *x = 0;
        *y = 3;
        break;
    case UniElementVStep:
        *x = 0;
        *y = preset->h <= 2 ? 4 : 0;
        break;
    case UniElementButton:
        *x = preset->w == 1 ? 1 : 0;
        *y = preset->h == 1 ? 5 : 4;
        break;
    }
    if(*x + preset->w > 3) *x = 3 - preset->w;
    if(*y + preset->h > 6) *y = 6 - preset->h;
}

static void remove_element_in_memory(UniRemote* remote, size_t index) {
    if(!remote || index >= remote->element_count) return;
    for(size_t i = index; i + 1 < remote->element_count; i++) {
        remote->elements[i] = remote->elements[i + 1];
    }
    remote->element_count--;
    memset(&remote->elements[remote->element_count], 0, sizeof(UniElement));
}

static void remove_overlaps(
    UniRemote* remote,
    size_t* protected_index,
    const UniElement* candidate,
    uint8_t x,
    uint8_t y) {
    if(!remote || !candidate) return;

    size_t i = 0;
    while(i < remote->element_count) {
        if(protected_index && i == *protected_index) {
            i++;
            continue;
        }

        const UniElement* e = &remote->elements[i];
        if(!elements_overlap_at(candidate, x, y, e, e->x, e->y)) {
            i++;
            continue;
        }

        remove_element_in_memory(remote, i);
        if(protected_index && i < *protected_index) (*protected_index)--;
    }
}

bool uni_remote_store_move_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    int8_t dx,
    int8_t dy) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count) return false;

    UniElement* moving = &remote->elements[element_index];
    const int nx = (int)moving->x + dx;
    const int ny = (int)moving->y + dy;
    if(nx < 0 || ny < 0 || nx + moving->w > 3 || ny + moving->h > 6) return false;

    uint8_t old_x[UNI_MAX_ELEMENTS] = {0};
    uint8_t old_y[UNI_MAX_ELEMENTS] = {0};
    bool collided[UNI_MAX_ELEMENTS] = {false};
    const size_t count = remote->element_count;

    for(size_t i = 0; i < count; i++) {
        old_x[i] = remote->elements[i].x;
        old_y[i] = remote->elements[i].y;
        if(i != element_index &&
           elements_overlap_at(
               moving,
               (uint8_t)nx,
               (uint8_t)ny,
               &remote->elements[i],
               remote->elements[i].x,
               remote->elements[i].y)) {
            collided[i] = true;
        }
    }

    const uint8_t preferred_x = moving->x;
    const uint8_t preferred_y = moving->y;
    moving->x = (uint8_t)nx;
    moving->y = (uint8_t)ny;

    for(size_t i = 0; i < count; i++) {
        if(!collided[i]) continue;

        UniElement* displaced = &remote->elements[i];
        uint8_t rx = 0;
        uint8_t ry = 0;
        if(!find_nearest_free_position(
               remote,
               i,
               displaced,
               preferred_x,
               preferred_y,
               &rx,
               &ry)) {
            for(size_t j = 0; j < count; j++) {
                remote->elements[j].x = old_x[j];
                remote->elements[j].y = old_y[j];
            }
            return false;
        }
        displaced->x = rx;
        displaced->y = ry;
    }

    if(!uni_remote_store_save(store, remote_index)) {
        for(size_t j = 0; j < count; j++) {
            remote->elements[j].x = old_x[j];
            remote->elements[j].y = old_y[j];
        }
        return false;
    }
    return true;
}

static bool find_free_position(
    const UniRemote* remote,
    const UniElement* candidate,
    uint8_t* out_x,
    uint8_t* out_y) {
    if(!candidate) return false;
    for(uint8_t y = 0; y + candidate->h <= 6; y++) {
        for(uint8_t x = 0; x + candidate->w <= 3; x++) {
            if(element_free(remote, UNI_MAX_ELEMENTS, candidate, x, y)) {
                *out_x = x;
                *out_y = y;
                return true;
            }
        }
    }
    return false;
}

static void init_from_preset(
    UniElement* element,
    const UniElementPreset* preset,
    const char* id,
    uint8_t page,
    uint8_t x,
    uint8_t y) {
    memset(element, 0, sizeof(UniElement));
    snprintf(element->id, sizeof(element->id), "%s", id ? id : preset->id);
    element->type = preset->type;
    element->page = page;
    element->x = x;
    element->y = y;
    element->w = preset->w;
    element->h = preset->h;
    snprintf(element->label, sizeof(element->label), "%s", preset->label);
    snprintf(element->icon, sizeof(element->icon), "%s", preset->icon);
}

bool uni_remote_store_add_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t preset_index,
    uint8_t page,
    size_t* new_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    const UniElementPreset* preset = uni_element_preset_get(preset_index);
    if(!remote || !preset) return false;

    if(page >= UNI_MAX_PAGES) return false;
    UniElement candidate = {0};
    candidate.type = preset->type;
    candidate.page = page;
    candidate.w = preset->w;
    candidate.h = preset->h;

    uint8_t x = 0;
    uint8_t y = 0;
    if(!find_free_position(remote, &candidate, &x, &y)) {
        /*
         * No valid placement: ADD becomes deliberate mask-region replacement.
         * A 3x3 D-pad removes only its five occupied cross cells; four corner
         * buttons survive because those cells are not part of the D-pad mask.
         */
        preferred_position(preset, &x, &y);
        remove_overlaps(remote, NULL, &candidate, x, y);
    }

    if(remote->element_count >= UNI_MAX_ELEMENTS) return false;
    const size_t index = remote->element_count++;
    UniElement* e = &remote->elements[index];
    char id[UNI_ID_MAX];
    snprintf(id, sizeof(id), "%.18s%lu", preset->id, (unsigned long)index);
    init_from_preset(e, preset, id, page, x, y);
    if(remote->page_count <= page) remote->page_count = (uint8_t)(page + 1);

    if(!uni_remote_store_save(store, remote_index)) {
        remote->element_count--;
        memset(&remote->elements[remote->element_count], 0, sizeof(UniElement));
        return false;
    }
    if(new_index) *new_index = index;
    return true;
}

bool uni_remote_store_replace_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    size_t preset_index,
    size_t* result_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    const UniElementPreset* preset = uni_element_preset_get(preset_index);
    if(!remote || !preset || element_index >= remote->element_count) return false;

    const UniElement old = remote->elements[element_index];
    uint8_t x = old.x;
    uint8_t y = old.y;
    if(x + preset->w > 3) x = 3 - preset->w;
    if(y + preset->h > 6) y = 6 - preset->h;

    UniElement candidate = old;
    candidate.type = preset->type;
    candidate.w = preset->w;
    candidate.h = preset->h;

    size_t protected_index = element_index;
    remove_overlaps(remote, &protected_index, &candidate, x, y);
    element_index = protected_index;

    if(preset->type == old.type) {
        remote->elements[element_index] = old;
        remote->elements[element_index].x = x;
        remote->elements[element_index].y = y;
        remote->elements[element_index].w = preset->w;
        remote->elements[element_index].h = preset->h;
    } else {
        init_from_preset(
            &remote->elements[element_index],
            preset,
            old.id,
            old.page,
            x,
            y);
    }

    const bool saved = uni_remote_store_save(store, remote_index);
    if(saved && result_index) *result_index = element_index;
    return saved;
}

bool uni_remote_store_remove_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count) return false;
    for(size_t i = element_index; i + 1 < remote->element_count; i++) {
        remote->elements[i] = remote->elements[i + 1];
    }
    remote->element_count--;
    memset(&remote->elements[remote->element_count], 0, sizeof(UniElement));
    return uni_remote_store_save(store, remote_index);
}

bool uni_remote_store_apply_layout(
    UniRemoteStore* store,
    size_t remote_index,
    size_t layout_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    const UniLayoutPreset* layout = uni_layout_preset_get(layout_index);
    if(!remote || !layout || layout->count > UNI_MAX_ELEMENTS) return false;

    memset(remote->elements, 0, UNI_MAX_ELEMENTS * sizeof(UniElement));
    memcpy(remote->elements, layout->elements, layout->count * sizeof(UniElement));
    remote->element_count = layout->count;
    remote->page_count = 1;
    return uni_remote_store_save(store, remote_index);
}

static char* binding_field(UniElement* e, const char* field) {
    if(strcmp(field, "tap") == 0) return e->tap;
    if(strcmp(field, "hold") == 0) return e->hold;
    if(strcmp(field, "up") == 0) return e->up;
    if(strcmp(field, "down") == 0) return e->down;
    if(strcmp(field, "left") == 0) return e->left;
    if(strcmp(field, "right") == 0) return e->right;
    if(strcmp(field, "ok") == 0) return e->ok;
    if(strcmp(field, "up_hold") == 0) return e->up_hold;
    if(strcmp(field, "down_hold") == 0) return e->down_hold;
    if(strcmp(field, "left_hold") == 0) return e->left_hold;
    if(strcmp(field, "right_hold") == 0) return e->right_hold;
    if(strcmp(field, "ok_hold") == 0) return e->ok_hold;
    return NULL;
}

bool uni_remote_store_set_binding(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    const char* field,
    const char* binding) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count || !field) return false;
    char* target = binding_field(&remote->elements[element_index], field);
    if(!target) return false;
    snprintf(target, UNI_BINDING_MAX, "%s", binding ? binding : "");
    return uni_remote_store_save(store, remote_index);
}

static char* icon_field(UniElement* e, const char* field) {
    if(strcmp(field, "icon") == 0) return e->icon;
    if(strcmp(field, "hold_icon") == 0) return e->hold_icon;
    if(strcmp(field, "up_hold_icon") == 0) return e->up_hold_icon;
    if(strcmp(field, "down_hold_icon") == 0) return e->down_hold_icon;
    if(strcmp(field, "left_hold_icon") == 0) return e->left_hold_icon;
    if(strcmp(field, "right_hold_icon") == 0) return e->right_hold_icon;
    if(strcmp(field, "ok_hold_icon") == 0) return e->ok_hold_icon;
    return NULL;
}

bool uni_remote_store_set_icon(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    const char* field,
    const char* icon_id) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count || !field) return false;
    char* target = icon_field(&remote->elements[element_index], field);
    if(!target) return false;
    snprintf(target, UNI_ICON_ID_MAX, "%s", icon_id ? icon_id : "");
    return uni_remote_store_save(store, remote_index);
}

bool uni_remote_store_set_hard_binding(
    UniRemoteStore* store,
    size_t remote_index,
    UniHardKeySlot slot,
    const char* binding) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || slot >= UniHardCount) return false;
    snprintf(
        remote->hard_bindings[slot],
        UNI_BINDING_MAX,
        "%s",
        binding ? binding : "");
    return uni_remote_store_save(store, remote_index);
}

bool uni_remote_store_import_ir(
    UniRemoteStore* store,
    size_t remote_index,
    const char* source_path) {
    if(!store || !source_path || !source_path[0]) return false;
    if(!uni_remote_store_load_details(store, remote_index)) return false;

    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || remote->transport != UniTransportInfrared) return false;

    FlipperFormat* source = flipper_format_file_alloc(store->storage);
    if(!source) return false;
    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    bool valid = false;
    if(flipper_format_file_open_existing(source, source_path)) {
        valid = flipper_format_read_header(source, filetype, &version) &&
                strcmp(furi_string_get_cstr(filetype), "IR signals file") == 0 &&
                version == 1U;
        flipper_format_file_close(source);
    }
    furi_string_free(filetype);
    flipper_format_free(source);
    if(!valid) return false;

    char dir[UNI_PATH_MAX];
    snprintf(dir, sizeof(dir), "%s", remote->config_path);
    char* slash = strrchr(dir, '/');
    if(!slash) return false;
    *slash = '\0';

    char destination[UNI_PATH_MAX];
    char temporary[UNI_PATH_MAX];
    snprintf(destination, sizeof(destination), "%.145s/signals.ir", dir);
    snprintf(temporary, sizeof(temporary), "%.145s/signals.tmp", dir);

    if(strcmp(source_path, destination) != 0) {
        storage_common_remove(store->storage, temporary);
        if(storage_common_copy(store->storage, source_path, temporary) != FSE_OK) {
            storage_common_remove(store->storage, temporary);
            return false;
        }

        storage_common_remove(store->storage, destination);
        if(storage_common_rename(store->storage, temporary, destination) != FSE_OK) {
            storage_common_remove(store->storage, temporary);
            return false;
        }
    }

    snprintf(remote->signal_file, sizeof(remote->signal_file), "signals.ir");
    snprintf(remote->signal_path, sizeof(remote->signal_path), "%s", destination);
    return uni_remote_store_save(store, remote_index);
}

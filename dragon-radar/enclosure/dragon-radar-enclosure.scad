// =============================================================================
//  Dragon Radar UWB — Enclosure (筐体) v0.1 skeleton
//  DigiKey Make ONE Challenge 2026
//
//  ターゲット: Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C (丸型一体モジュール)
//  印刷: FDM / PLA / ベッド ~220mm (ø一体造形)
//
//  座標系:
//    +Z = 画面側 (ユーザーに向く向き)。Z=0 は前面ガラス表面。
//    モジュール本体は -Z 方向へ伸びる。
//    +Y = 筐体「上」(ボタンを配置する側)。
//
//  ⚠ 寸法の出典と確度:
//    [確定] Waveshare wiki 寸法図 + CNX 記事より
//      - ガラス外形 ø115, 有効表示 ø87.6 (800x800, 3.4")
//      - スタック総厚 ~15mm (前面ガラス~6mm + 背面基板/コネクタ~9mm)
//    [要実測 = MEASURE] ボードが届いたら現物で確定すること:
//      - マウント穴の PCD・本数・径 (現状 4穴/ø105/ø2.5 と仮置き)
//      - USB-C / USB-OTG / microSD のリム角度位置と高さ
//      - 背面コネクタ群の最大突出 (back_clear に効く)
//    実測したら下の MEASURE 印の変数だけ直せば全体が追従します。
// =============================================================================

// ---- レンダリング対象の選択 -------------------------------------------------
//  "assembly"  : 全部品 + ボード mock を重ねて嵌合確認 (色分け)
//  "bezel"     : 前面ベゼルリングのみ (印刷用)
//  "body"      : 本体タブのみ (印刷用)
//  "back"      : バックカバーのみ (印刷用)
//  "board"     : ボード mock のみ (確認用)
part = "assembly";

$fn = 120;                 // 円弧分割。最終出力前は 180+ 推奨

// ====================== ボード (3.4C) 実寸パラメータ =========================
board_glass_dia   = 115.0; // ガラス/前面外形 [確定]
board_active_dia  = 87.6;  // 有効表示エリア径 [確定]
board_glass_th    = 6.0;   // 前面ガラス厚 [確定]
board_stack_th    = 15.0;  // モジュール総厚 (ガラス前面〜背面最深部) [確定]
board_pcb_dia     = 108.0; // 背面 PCB 外径 (ガラスより少し小さい想定) [概算]

// マウント穴 [MEASURE] ----- 現物で要確定
mount_count       = 4;     // 穴数
mount_pcd         = 105.0; // ボルト円直径 (PCD)
mount_hole_dia    = 2.5;   // ネジ下穴 (M2.5 セルフタップ想定)
mount_angle0      = 45;    // 1本目の角度オフセット [deg]

// 背面コネクタの突出余裕 [MEASURE]
back_clear        = 8.0;   // PCB 背面〜バックカバー内面の隙間 (電池/配線/コネクタ)

// ====================== 筐体パラメータ =======================================
wall              = 2.4;   // 側壁厚 (FDM: 0.4ノズル×6)
side_clear        = 0.6;   // モジュール外周と内壁のクリアランス (片側)
bezel_overlap     = 2.0;   // ベゼルがガラス前面を抑える掛かり代 (半径方向)
bezel_face_th     = 2.5;   // 前面ベゼルの板厚
lip_h             = 5.0;   // ベゼル⇔本体の嵌合リップ高さ
lip_gap           = 0.3;   // 嵌合クリアランス
back_th           = 2.4;   // バックカバー板厚

// 派生寸法
inner_dia  = board_glass_dia + 2*side_clear;        // 内壁径 ≈ 116.2
outer_dia  = inner_dia + 2*wall;                    // 外径 ≈ 121.0
window_dia = board_active_dia + 2*bezel_overlap;    // 窓径 ≈ 91.6 (有効表示は完全露出)

// 深さ方向 (Z): 前面ガラス Z=0 → 背面方向へ
//   ベゼル前面:  Z = +bezel_face_th .. 0
//   モジュール:  Z = 0 .. -board_stack_th
//   背面余裕:    -board_stack_th .. -(board_stack_th+back_clear)
//   バック板:    その後ろ back_th
depth_inner = board_stack_th + back_clear;          // 内寸深さ ≈ 23
outer_depth = bezel_face_th + depth_inner + back_th;// 筐体総厚 ≈ 27.9 (目標30mm内)

// ボタン (MKBKLLJY ø12mm メタル) [筐体上部のリムに配置]
button_dia        = 12.4;  // ø12 + クリアランス
button_z          = -board_stack_th/2; // リム高さ中央あたり (暫定)

echo(str("outer_dia = ", outer_dia, " mm"));
echo(str("outer_depth = ", outer_depth, " mm (目標 30 以内)"));
echo(str("window_dia = ", window_dia, " mm"));

// ============================================================================
//  ボード mock (嵌合確認用、印刷対象ではない)
// ============================================================================
module board_mock() {
    color("DimGray") {
        // 前面ガラス
        translate([0,0,-board_glass_th])
            cylinder(h=board_glass_th, d=board_glass_dia);
        // 有効表示 (黒)
        color("Black")
            translate([0,0,-0.01]) cylinder(h=0.02, d=board_active_dia);
        // 背面 PCB スタック
        translate([0,0,-board_stack_th])
            cylinder(h=board_stack_th-board_glass_th, d=board_pcb_dia);
    }
    // マウント穴位置の可視化 (ピン)
    color("Gold")
    for (i=[0:mount_count-1])
        rotate([0,0, mount_angle0 + i*360/mount_count])
            translate([mount_pcd/2, 0, -board_stack_th])
                cylinder(h=board_stack_th, d=mount_hole_dia);
}

// ============================================================================
//  前面ベゼルリング
//   - 前面板 (窓あき) + 外スカート + 本体に被さるリップ
// ============================================================================
module front_bezel() {
    difference() {
        union() {
            // 前面板リング
            translate([0,0,0])
                cylinder(h=bezel_face_th, d=outer_dia);
            // 外スカート (本体外周に被せて -Z へ)
            translate([0,0,-lip_h])
                difference() {
                    cylinder(h=lip_h+bezel_face_th, d=outer_dia);
                    translate([0,0,-0.1])
                        cylinder(h=lip_h+bezel_face_th+0.2, d=outer_dia-2*wall);
                }
        }
        // 表示窓 (貫通)
        translate([0,0,-0.1])
            cylinder(h=bezel_face_th+0.2, d=window_dia);
        // ガラスを逃がす座ぐり (前面板の裏側、ガラス外周を受ける)
        translate([0,0,-0.01])
            cylinder(h=bezel_face_th, d=board_glass_dia+2*lip_gap);
    }
}

// ============================================================================
//  本体タブ (rear tub)
//   - 側壁 + モジュール着座シェルフ + マウントボス + コネクタ/ボタン開口
// ============================================================================
module mount_bosses() {
    for (i=[0:mount_count-1])
        rotate([0,0, mount_angle0 + i*360/mount_count])
            translate([mount_pcd/2, 0, -depth_inner])
                difference() {
                    cylinder(h=depth_inner-board_stack_th+ (board_stack_th-board_glass_th), d=6);
                    // ネジ下穴 (バックから or 前から、暫定で貫通)
                    translate([0,0,-0.1])
                        cylinder(h=depth_inner+0.2, d=mount_hole_dia);
                }
}

module connector_cutout(angle, w, h) {
    // リム壁に矩形開口。angle=配置角[deg], w=幅, h=高さ(Z)
    rotate([0,0,angle])
        translate([inner_dia/2 - 0.1, 0, button_z])
            rotate([0,90,0])
                // 中心を壁厚中央に
                translate([0,0,-(wall+1)/2])
                    linear_extrude(wall+2)
                        square([h, w], center=true);
}

module main_body() {
    difference() {
        union() {
            // 側壁 (前面ベゼル下端 0 から depth_inner まで)
            translate([0,0,-depth_inner])
                difference() {
                    cylinder(h=depth_inner, d=outer_dia);
                    translate([0,0,-0.1])
                        cylinder(h=depth_inner+0.2, d=inner_dia);
                }
            // モジュール着座シェルフ (ガラス背面を受ける内向きフランジ)
            translate([0,0,-board_glass_th])
                difference() {
                    cylinder(h=2, d=inner_dia);
                    translate([0,0,-0.1])
                        cylinder(h=2.2, d=board_glass_dia-2); // 2mm 掛かり
                }
            // マウントボス
            mount_bosses();
        }
        // --- 開口群 ---
        // [MEASURE] 角度はすべて仮。現物で USB/SD のリム位置を確定すること
        connector_cutout(0,   10, 4);   // USB-C (PWR/UART) 右側 仮
        connector_cutout(20,  10, 4);   // USB-OTG 仮
        connector_cutout(-25, 13, 3.5); // microSD 仮
        // ボタン穴 (上部リム +Y)
        rotate([0,0,90])
            translate([inner_dia/2 - wall - 1, 0, button_z])
                rotate([0,90,0])
                    cylinder(h=wall+3, d=button_dia, center=true);
    }
}

// ============================================================================
//  バックカバー
// ============================================================================
module back_cover() {
    translate([0,0,-depth_inner-back_th])
    difference() {
        union() {
            cylinder(h=back_th, d=outer_dia);
            // 本体内側に嵌るリップ
            translate([0,0,back_th-0.01])
                cylinder(h=lip_h, d=inner_dia-2*lip_gap);
        }
        // ネジ穴 (マウントボスに合わせる)
        for (i=[0:mount_count-1])
            rotate([0,0, mount_angle0 + i*360/mount_count])
                translate([mount_pcd/2, 0, -0.1])
                    cylinder(h=back_th+lip_h+0.2, d=mount_hole_dia+1.0);
        // 配線/通気 (中央スリット) — 仮
        for (a=[0:60:359])
            rotate([0,0,a])
                translate([outer_dia/4,0,-0.1])
                    cylinder(h=back_th+0.2, d=3);
    }
}

// ============================================================================
//  ディスパッチ
// ============================================================================
if (part == "assembly") {
    color("SteelBlue", 0.85) front_bezel();
    color("LightGray",  0.6) main_body();
    color("Tan",        0.8) back_cover();
    board_mock();
} else if (part == "bezel") {
    front_bezel();
} else if (part == "body") {
    main_body();
} else if (part == "back") {
    back_cover();
} else if (part == "board") {
    board_mock();
}

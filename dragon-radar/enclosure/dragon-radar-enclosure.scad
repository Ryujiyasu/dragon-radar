// =============================================================================
//  Dragon Radar UWB — Enclosure (筐体) v0.2
//  DigiKey Make ONE Challenge 2026
//
//  ターゲット: Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C (丸型一体モジュール)
//  印刷: FDM / PLA / ベッド ~220mm (ø一体造形)
//
//  v0.2 変更点:
//    - ベゼル⇔本体: 環状スナップフィット (連続ビード+溝)。前面ネジ無し。
//    - 本体上端を段付き「トング」化し、スカートを被せて外周 ø121 を面一に。
//    - バックカバー: M3 セルフタップ 4本留め (後方空洞のボスに固定)。
//    - v0.1 の嵌合バグ修正 (スカートと壁の干渉 / ガラス前面の押さえ面欠落)。
//
//  座標系:
//    +Z = 画面側 (ユーザーに向く向き)。Z=0 は前面ガラス表面。
//    モジュール本体は -Z 方向へ伸びる。 +Y = 筐体「上」(ボタン側)。
//
//  ⚠ 寸法の出典: README.md 参照。[MEASURE] 印は現物到着後に実測確定する仮値。
// =============================================================================

// ---- レンダリング対象 -------------------------------------------------------
//  assembly / bezel / body / back / board
part = "assembly";
clip = false;              // true: Y>=0 半割り断面表示 (嵌合確認用、非出力)

$fn = 120;                 // 最終出力前は 180+ 推奨

// ====================== ボード (3.4C) 実寸 ===================================
board_glass_dia   = 115.0; // ガラス/前面外形 [確定]
board_active_dia  = 87.6;  // 有効表示エリア径 [確定]
board_glass_th    = 6.0;   // 前面ガラス厚 [確定]
board_stack_th    = 15.0;  // モジュール総厚 [確定]
board_pcb_dia     = 108.0; // 背面 PCB 外径 [概算]

// マウント穴 [MEASURE]
mount_count       = 4;
mount_pcd         = 105.0;
mount_hole_dia    = 2.5;
mount_angle0      = 45;

back_clear        = 8.0;   // PCB 背面〜バックカバー内面の隙間 [MEASURE]

// ====================== 筐体基本パラメータ ===================================
wall              = 2.4;   // 側壁厚
side_clear        = 0.6;   // モジュール外周と内壁のクリアランス(片側)
bezel_overlap     = 2.0;   // ベゼルがガラス前面を抑える掛かり代(径方向)
bezel_face_th     = 2.5;   // 前面ベゼル板厚
back_th           = 2.4;   // バックカバー板厚

// 派生
inner_dia  = board_glass_dia + 2*side_clear;     // 内壁径 ≈116.2
outer_dia  = inner_dia + 2*wall;                 // 外径 ≈121.0
window_dia = board_active_dia + 2*bezel_overlap; // 窓径 ≈91.6
R_out      = outer_dia/2;
R_in       = inner_dia/2;

// 深さ(Z)
depth_inner = board_stack_th + back_clear;       // 内寸深さ ≈23
outer_depth = bezel_face_th + depth_inner + back_th; // 総厚 ≈27.9 (目標30以内)

// ---- 固定方式: ベゼル環状スナップ ------------------------------------------
skirt_wall  = 2.0;         // ベゼルスカート厚
tongue_h    = 7.0;         // 本体上端トング高さ (スカートが被る)
fit_gap     = 0.3;         // トング外面とスカート内面の摺動隙
R_tongue    = R_out - skirt_wall;        // 本体トング外半径
R_skirt_in  = R_tongue + fit_gap;        // ベゼルスカート内半径
snap_bead   = 0.9;         // スナップビード/溝の径方向量
snap_h      = 1.6;         // ビード/溝の軸方向高さ
snap_lead   = 0.8;         // リードイン面取り
snap_z      = -tongue_h + 3.0;           // ビード/溝の中心Z

// ---- バックカバー ネジ (M3 セルフタップ) -----------------------------------
screw_dia    = 3.0;
screw_pilot  = 2.6;        // PLA セルフタップ下穴
screw_head   = 6.0;        // 頭ザグリ径
screw_head_h = 2.6;
boss_dia     = 7.5;
screw_n      = 4;
screw_a0     = 45;
screw_pcd    = inner_dia - boss_dia - 1.0;  // ≈107.7 (後方空洞、壁際)

// ボタン (MKBKLLJY ø12mm) [上部リム]
button_dia        = 12.4;
button_z          = -board_stack_th/2; // 暫定

echo(str("outer_dia=", outer_dia, "  outer_depth=", outer_depth, "  window_dia=", window_dia));

// ============================================================================
//  共通ヘルパ: 環状ビード/溝のプロファイル (rotate_extrude)
// ============================================================================
// 内向きビード: 半径 Ri の内面から軸方向に bead だけ突出 (lead 付き台形)
module bead_inward(Ri, bead, h, lead) {
    rotate_extrude()
        polygon([[Ri, -h/2-lead], [Ri-bead, -h/2],
                 [Ri-bead, h/2],   [Ri, h/2+lead]]);
}
// 外向き溝(切削体): 半径 Ro の外面を bead だけ凹ませる矩形リング
module groove_solid(Ro, bead, h) {
    rotate_extrude()
        polygon([[Ro-bead, -h/2], [Ro+1, -h/2],
                 [Ro+1, h/2],     [Ro-bead, h/2]]);
}

// ============================================================================
//  ボード mock (嵌合確認用、非印刷)
// ============================================================================
module board_mock() {
    color("DimGray") {
        translate([0,0,-board_glass_th]) cylinder(h=board_glass_th, d=board_glass_dia);
        color("Black") translate([0,0,-0.01]) cylinder(h=0.02, d=board_active_dia);
        translate([0,0,-board_stack_th])
            cylinder(h=board_stack_th-board_glass_th, d=board_pcb_dia);
    }
    color("Gold")
    for (i=[0:mount_count-1])
        rotate([0,0, mount_angle0 + i*360/mount_count])
            translate([mount_pcd/2, 0, -board_stack_th])
                cylinder(h=board_stack_th, d=mount_hole_dia);
}

// ============================================================================
//  前面ベゼル (面板 + 段付きスカート + 内向きスナップビード)
// ============================================================================
module front_bezel() {
    difference() {
        union() {
            // 面板リング (窓〜外周)
            cylinder(h=bezel_face_th, d=outer_dia);
            // 段付きスカート (外周 R_out、内 R_skirt_in)
            translate([0,0,-tongue_h])
                difference() {
                    cylinder(h=tongue_h, d=outer_dia);
                    translate([0,0,-0.1]) cylinder(h=tongue_h+0.2, d=2*R_skirt_in);
                }
            // スナップビード (スカート内面、内向き)
            translate([0,0, snap_z]) bead_inward(R_skirt_in, snap_bead, snap_h, snap_lead);
        }
        // 表示窓
        translate([0,0,-0.1]) cylinder(h=bezel_face_th+0.2, d=window_dia);
    }
}

// ============================================================================
//  本体タブ (側壁 + 段付きトング + 着座シェルフ + バックカバー用ボス
//             + コネクタ/ボタン開口)
// ============================================================================
module screw_bosses(with_pilot=true) {
    for (i=[0:screw_n-1])
        rotate([0,0, screw_a0 + i*360/screw_n])
            translate([screw_pcd/2, 0, -depth_inner])
                difference() {
                    cylinder(h=back_clear, d=boss_dia); // 後方空洞に立つ
                    if (with_pilot)
                        translate([0,0,-0.1])
                            cylinder(h=back_clear+0.2, d=screw_pilot);
                }
}

module connector_cutout(angle, w, h) {
    rotate([0,0,angle])
        translate([R_in - 0.1, 0, button_z])
            rotate([0,90,0])
                translate([0,0,-(wall+1)/2])
                    linear_extrude(wall+2)
                        square([h, w], center=true);
}

module main_body() {
    difference() {
        union() {
            // 側壁 (Z = -depth_inner .. 0)。上端 tongue_h は外径を段付き(トング)
            translate([0,0,-depth_inner])
                difference() {
                    union() {
                        // 下部フル外径
                        cylinder(h=depth_inner-tongue_h, d=outer_dia);
                        // 上部トング (外径を skirt_wall ぶん縮める)
                        translate([0,0,depth_inner-tongue_h])
                            cylinder(h=tongue_h, d=2*R_tongue);
                    }
                    translate([0,0,-0.1]) cylinder(h=depth_inner+0.2, d=inner_dia);
                }
            // 着座シェルフ (ガラス背面を受ける内向きフランジ)
            translate([0,0,-board_glass_th])
                difference() {
                    cylinder(h=2, d=inner_dia);
                    translate([0,0,-0.1]) cylinder(h=2.2, d=board_glass_dia-2);
                }
            // バックカバー用ボス
            screw_bosses();
        }
        // トング外面のスナップ溝
        translate([0,0, snap_z]) groove_solid(R_tongue, snap_bead, snap_h+0.4);
        // トング上端外エッジに camming 面取り
        translate([0,0,-0.01])
            difference() {
                cylinder(h=snap_lead+0.2, r1=R_tongue, r2=R_tongue);
                cylinder(h=snap_lead+0.2, r1=R_tongue-snap_lead, r2=R_tongue+0.5);
            }
        // --- 開口群 (角度は [MEASURE] 仮) ---
        connector_cutout(0,   10, 4);   // USB-C 仮
        connector_cutout(20,  10, 4);   // USB-OTG 仮
        connector_cutout(-25, 13, 3.5); // microSD 仮
        // ボタン穴 (上部 +Y)
        rotate([0,0,90])
            translate([R_in - wall - 1, 0, button_z])
                rotate([0,90,0]) cylinder(h=wall+3, d=button_dia, center=true);
    }
}

// ============================================================================
//  バックカバー (ネジ留め + 嵌合リップ + 通気)
// ============================================================================
module back_cover() {
    translate([0,0,-depth_inner-back_th])
    difference() {
        union() {
            cylinder(h=back_th, d=outer_dia);
            // 本体内側に嵌るリップ
            translate([0,0,back_th-0.01])
                cylinder(h=3.0, d=inner_dia-2*fit_gap);
        }
        // ネジ穴: 背面からザグリ + 貫通
        for (i=[0:screw_n-1])
            rotate([0,0, screw_a0 + i*360/screw_n])
                translate([screw_pcd/2, 0, 0]) {
                    translate([0,0,-0.1]) cylinder(h=back_th+3.2, d=screw_dia+0.6);
                    translate([0,0,-0.1]) cylinder(h=screw_head_h, d=screw_head); // 頭ザグリ
                }
        // 通気/配線
        for (a=[0:60:359])
            rotate([0,0,a])
                translate([outer_dia/4,0,-0.1]) cylinder(h=back_th+0.2, d=3);
    }
}

// ============================================================================
//  ディスパッチ
// ============================================================================
module dispatch() {
    if (part == "assembly") {
        color("SteelBlue", 0.85) front_bezel();
        color("LightGray",  0.55) main_body();
        color("Tan",        0.8) back_cover();
        board_mock();
    } else if (part == "bezel") { front_bezel();
    } else if (part == "body")  { main_body();
    } else if (part == "back")  { back_cover();
    } else if (part == "board") { board_mock();
    }
}

if (clip)
    intersection() { dispatch(); translate([-200,0,-200]) cube([400,200,400]); }
else
    dispatch();

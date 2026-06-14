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
board_stack_th    = 14.0;  // モジュール総厚 [実測 2026-05-31]
// 背面キャリア基板 (四角) [実測 2026-05-31: 65×86]
carrier_x         = 86.0;  // 長辺
carrier_y         = 65.0;  // 短辺
carrier_angle     = 180;   // 取付向き: USB-C(上辺)を筐体底(270°)へ→ USB-A右0/microSD左180
carrier_th        = board_stack_th - board_glass_th; // ガラス背面〜最深部 ≈8

// マウント穴 (四隅ではない) [MEASURE]
//   キャリア中心を原点、長辺=X として [x,y] で実測位置を列挙する。
//   実測 2026-05-31 (すべて外形込み→穴芯はスタンドオフ径ぶん内側): 上下非対称な台形。
//   列間X=83.6 / 左列Y間隔32(上端→上ネジ8.6) / 右列Y間隔39(上端→上ネジ8.3)。
//   中心原点・上端+32.5。左上=32.5-8.6=23.9, 右上=32.5-8.3=24.2。
mount_holes       = [[-41.8, 23.9],[-41.8, -8.1],   // 左列: Y間隔32
                     [ 41.8, 24.2],[ 41.8,-14.8]];  // 右列: Y間隔39
mount_hole_dia    = 2.5;

back_clear        = 8.0;   // PCB 背面〜バックカバー内面の隙間 [MEASURE]

// ====================== 筐体基本パラメータ ===================================
//  素材: Bambu A1 / PLA Matte (FDM)。2026-06-14 実機クーポンで検証:
//   薄壁(wall3.2)スナップは爪が薄く(~1.2mm)ビードも浅く保持せず→壁厚化で爪剛性+噛み深さUP
//   ボタン穴12.3=ぴったり / 嵌合は厚壁版(下記)でカチッと保持を確認
wall              = 4.4;   // 側壁厚 (トング壁2.2/爪剛性確保。薄壁だとスナップが保持しない)
side_clear        = 0.6;   // モジュール外周と内壁のクリアランス(片側)
bezel_overlap     = 2.0;   // ベゼルがガラス前面を抑える掛かり代(径方向)
bezel_face_th     = 2.5;   // 前面ベゼル板厚
back_th           = 2.4;   // バックカバー板厚

// 派生
inner_dia  = board_glass_dia + 2*side_clear;     // 内壁径 ≈116.2
outer_dia  = inner_dia + 2*wall;                 // 外径 ≈125.0 (wall=4.4, スナップ強化で増)
window_dia = board_active_dia + 2*bezel_overlap; // 窓径 ≈91.6
R_out      = outer_dia/2;
R_in       = inner_dia/2;

// 深さ(Z)
depth_inner = board_stack_th + back_clear;       // 内寸深さ ≈23
outer_depth = bezel_face_th + depth_inner + back_th; // 総厚 ≈27.9 (目標30以内)

// ---- 固定方式: ベゼル環状スナップ ------------------------------------------
skirt_wall  = 2.2;         // ベゼルスカート厚(爪実厚=skirt_wall-fit_gap≈1.9mm。薄いと保持せず)
tongue_h    = 7.0;         // 本体上端トング高さ (スカートが被る)
fit_gap     = 0.30;        // 摺動隙。FDM/PLA実機で確定(2026-06-14)。020/030/040は体感差小=頑丈
R_tongue    = R_out - skirt_wall;        // 本体トング外半径
R_skirt_in  = R_tongue + fit_gap;        // ベゼルスカート内半径
snap_bead   = 1.2;         // ビード/溝量。保持代=snap_bead-fit_gap=0.9(厚壁トングで深く噛む)
snap_h      = 1.6;         // ビード/溝の軸方向高さ
snap_lead   = 1.0;         // リードイン面取り(camを軽く: 0.8→1.0)
snap_z      = -tongue_h + 3.0;           // ビード/溝の中心Z
// ベゼルスカートをスリットで分割しバネ爪化(「バネ」対策)。snap_slots=0 で無効
snap_slots  = 12;          // バネ爪の本数
snap_slot_w = 1.4;         // スリット幅(周方向)
slot_z_top  = -1.5;        // フィンガー根元を残すZ (face側)
slot_z_bot  = -(tongue_h + 1.0); // 下端を確実に切る
snap_groove_h = snap_h + 1.4;    // 溝の軸方向を太く(ビードに遊び=「幅溝を太く」対策)

// ---- バックカバー ネジ (M3 セルフタップ) -----------------------------------
screw_dia    = 3.0;
screw_pilot  = 2.6;        // PLA セルフタップ下穴
screw_head   = 6.0;        // 頭ザグリ径
screw_head_h = 2.6;
boss_dia     = 7.5;
screw_n      = 4;
screw_a0     = 45;
screw_pcd    = inner_dia - boss_dia + 3.0;  // ≈111.7 ボスを壁に食い込ませ一体化 (浮き防止)

// ---- ボード固定 (バックカバー→台形マウント穴へ締結) [VERIFY] ----
//   カラーでボード⇔ディスプレイ締結済の上に、バックから4本でサンドイッチを筐体固定。
board_screw_dia    = 2.5;   // M2.5 想定 (現物の穴径で確定)
board_boss_dia     = 7.5;   // 受けボス外径 (頭ザグリ周囲に≥1mm壁を残すため太く)
board_screw_head   = 5.2;   // 頭ザグリ径
board_screw_head_h = 2.4;

// ---- ボタン (MKBKLLJY ø12 メタル momentary + 緑LEDリング, 12V) -------------
//  天面に突出する中空タレットに収める (端子はタレット内、配線のみ内部へ)
//  [VERIFY] 現物 (Phase4-5 で配線済) で寸法確定すること
button_angle     = 90;     // +Y = 天面 (12時方向)
button_z         = -11.0;  // タレット中心Z (本体壁内に収める)
button_dia       = 12.3;   // パネル穴。テスト片で12.3=ぴったり確認(2026-06-03)
// 実測 2026-05-31: 全長24 / 頭(押せる部分)1.7 / 頭先端〜ナット下端6mm
turret_bore      = 16.5;   // タレット内径 (ナット/端子逃げ) [VERIFY: #2 ナット最大径待ち]
turret_wall      = 2.6;    // タレット肉厚
turret_od        = turret_bore + 2*turret_wall; // ≈21.7
turret_protrude  = 14.0;   // 突出長。金属取付部を保持、端子+配線(柔)は内部隙間へ逃がす
turret_cap_th    = 1.8;    // 端面(パネル)厚。ナットねじ代の余裕確保(2.0→1.8) [VERIFY]
turret_clear     = 0.4;    // ベゼル逃げクリアランス
wire_hole_dia    = 5.0;    // 配線通路径

// コネクタ開口の Z (ボタンとは独立)
conn_z           = -(board_glass_th + carrier_th/2); // キャリア深さ中央 ≈-10

// ---- フィンガースカラップ (親指レスト) -------------------------------------
//  薄壁(2.4)+内側モジュールのため深掘り不可 → 局所肉盛りレンズに浅い凹みを彫る。
//  両手持ちの 3/9 時 (0/180°)。空にすれば無効。
scallop_angles   = [30, 150]; // コネクタ(0/180)を避け上側へ。両手親指位置
scallop_z        = -14.5;  // 高さ中央 (ベゼル占有域 z≤-7 より下に収める)
scallop_h        = 11.0;   // 縦長 (Z方向)。top≈-9 でベゼルと非干渉
scallop_padR     = 16.0;   // パッド球半径 (footprint 幅を決める)
scallop_proud    = 3.0;    // 盛り上げ高さ
scallop_cav_r    = 18.0;   // 凹み(縦シリンダ)半径
scallop_depth    = 3.0;    // 凹み深さ (パッド頂点から、floor は元壁面付近で薄肉化させない)

echo(str("outer_dia=", outer_dia, "  outer_depth=", outer_depth, "  window_dia=", window_dia));

// ============================================================================
//  共通ヘルパ: 環状ビード/溝のプロファイル (rotate_extrude)
// ============================================================================
// 内向きビード: 半径 Ri の内面から軸方向に bead だけ突出 (lead 付き台形)
module bead_inward(Ri, bead, h, lead) {
    // 外縁を +0.8 スカート側へ食い込ませ確実に一体化 (面一接触の別ソリッド化を防止)
    rotate_extrude()
        polygon([[Ri+0.8, -h/2-lead], [Ri-bead, -h/2],
                 [Ri-bead, h/2],   [Ri+0.8, h/2+lead]]);
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
    // 前面: 丸ディスプレイ (ガラス ø115) + 有効表示
    color("DimGray")
        translate([0,0,-board_glass_th]) cylinder(h=board_glass_th, d=board_glass_dia);
    color("Black") translate([0,0,-0.01]) cylinder(h=0.02, d=board_active_dia);
    // 背面: 四角キャリア基板 (65×86)
    color("DarkGreen")
        rotate([0,0,carrier_angle])
            translate([-carrier_x/2, -carrier_y/2, -board_stack_th])
                cube([carrier_x, carrier_y, carrier_th]);
    // マウント穴 (実測リスト)
    color("Gold")
        rotate([0,0,carrier_angle])
            for (h=mount_holes)
                translate([h[0], h[1], -board_stack_th])
                    cylinder(h=carrier_th, d=mount_hole_dia);
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
        // 天面タレットの逃げ (本体タレットがベゼルを貫通)
        turret_envelope(turret_od + 2*turret_clear, extra_len=0.1);
        // バネ爪スリット (スカートを分割して各爪が独立にたわむ)
        bezel_spring_slots();
    }
}

// ベゼルスカートを放射スリットで分割しバネ爪化
module bezel_spring_slots() {
    if (snap_slots > 0)
        for (i = [0:snap_slots-1])
            rotate([0,0, i*360/snap_slots])
                translate([(R_skirt_in + R_out)/2, 0, (slot_z_top + slot_z_bot)/2])
                    cube([(R_out - R_skirt_in) + 4, snap_slot_w,
                          slot_z_top - slot_z_bot], center=true);
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
    // 空洞内(R_in-2)から外周外(R_out+1)まで貫通。壁厚非依存(wall増でも外皮を残さない)
    rotate([0,0,angle])
        translate([R_in - 2, 0, conn_z])
            rotate([0,90,0])
                linear_extrude(R_out - R_in + 3)
                    square([h, w], center=true);
}

// ---- 天面ボタンタレット ----------------------------------------------------
// 外殻エンベロープ (本体に union / ベゼルに clearance subtract で共用)
module turret_envelope(d, extra_len=0) {
    rotate([0,0,button_angle])
        translate([R_in-2, 0, button_z])
            rotate([0,90,0])
                cylinder(h=(R_out+turret_protrude)-(R_in-2)+extra_len, d=d);
}
// タレットの中ぐり (メインボア + 端面ボタン穴)
module turret_bore_cut() {
    rotate([0,0,button_angle])
        translate([R_in-2, 0, button_z])
            rotate([0,90,0]) {
                // メインボア (内側へ3mm延ばして空洞へ開口)
                translate([0,0,-3])
                    cylinder(h=(R_out+turret_protrude)-(R_in-2)-turret_cap_th+3, d=turret_bore);
                // 端面のパネル穴 (ボタン軸)
                cylinder(h=(R_out+turret_protrude)-(R_in-2)+0.1, d=button_dia);
            }
}
// 配線通路: タレット内端から後方空洞へ -Z に降ろす (環状隙間を通す)
module turret_wire_cut() {
    rotate([0,0,button_angle])
        translate([R_in-1, 0, -depth_inner-1])
            cylinder(h=button_z-(-depth_inner-1)+0.1, d=wire_hole_dia);
}

// ---- フィンガースカラップ ----
// 肉盛りパッド (空洞を埋めないよう R>=R_in に制限)
module scallop_pads() {
    for (a=scallop_angles)
        rotate([0,0,a])
            intersection() {
                hull()
                    for (zz=[-scallop_h/2, scallop_h/2])
                        translate([R_out - scallop_padR + scallop_proud, 0, scallop_z+zz])
                            sphere(r=scallop_padR);
                // R_in 以上の環状シェルだけ残す
                difference() {
                    cylinder(h=4*outer_depth, d=4*outer_dia, center=true);
                    cylinder(h=4*outer_depth+1, d=inner_dia, center=true);
                }
                // Z スラブ: 前面(0)/背面(-depth_inner)を突き抜けない様 z∈[-(depth_inner-1),-1]
                translate([0,0,-depth_inner/2])
                    cube([4*outer_dia, 4*outer_dia, depth_inner-2], center=true);
            }
}
// 凹み (縦シリンダで親指レストを掘る)
module scallop_cuts() {
    for (a=scallop_angles)
        rotate([0,0,a])
            translate([R_out + scallop_proud + scallop_cav_r - scallop_depth, 0, scallop_z])
                cylinder(h=scallop_h+8, r=scallop_cav_r, center=true);
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
            // 着座シェルフ (ガラス背面を受ける内向きフランジ。上面=ガラス背面 z=-6)
            translate([0,0,-board_glass_th-2])
                difference() {
                    cylinder(h=2, d=inner_dia);
                    translate([0,0,-0.1]) cylinder(h=2.2, d=board_glass_dia-2);
                }
            // バックカバー用ボス
            screw_bosses();
            // 天面ボタンタレット (中実エンベロープ)
            turret_envelope(turret_od);
            // フィンガースカラップの肉盛りパッド
            scallop_pads();
        }
        // トング外面のスナップ溝 (軸方向を太く=ビードに遊び)
        translate([0,0, snap_z]) groove_solid(R_tongue, snap_bead, snap_groove_h);
        // トング上端外エッジに camming 面取り
        translate([0,0,-0.01])
            difference() {
                cylinder(h=snap_lead+0.2, r1=R_tongue, r2=R_tongue);
                cylinder(h=snap_lead+0.2, r1=R_tongue-snap_lead, r2=R_tongue+0.5);
            }
        // --- 開口群 (carrier_angle=180 に対応。角度の微調整は現物合わせ) ---
        connector_cutout(270, 22, 5.0); // USB-C ×2 (キャリア上辺→筐体底)
        connector_cutout(0,   13, 6.5); // USB-A OTG (キャリア左辺→筐体右。プラグ殻12mm+余裕)
        connector_cutout(180, 14, 3.5); // microSD (キャリア右辺→筐体左)
        // 天面タレットの中ぐり + 配線通路
        turret_bore_cut();
        turret_wire_cut();
        // フィンガースカラップの凹み
        scallop_cuts();
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
            // 本体内側に嵌るリップ (閉じネジ穴外縁との一致=退化を避け -1.5 小さく)
            translate([0,0,back_th-0.01])
                cylinder(h=3.0, d=inner_dia-2*fit_gap-1.5);
            // ボード固定ボス (プレートを貫通=z0から立ち上げ一体化、キャリア背面z=-14を受ける)
            rotate([0,0,carrier_angle])
                for (h=mount_holes)
                    translate([h[0], h[1], 0])
                        cylinder(h=back_th + depth_inner-board_stack_th, d=board_boss_dia);
        }
        // 本体閉じ用ネジ穴: 背面からザグリ + 貫通
        for (i=[0:screw_n-1])
            rotate([0,0, screw_a0 + i*360/screw_n])
                translate([screw_pcd/2, 0, 0]) {
                    translate([0,0,-0.1]) cylinder(h=back_th+3.2, d=screw_dia+0.6);
                    translate([0,0,-0.1]) cylinder(h=screw_head_h, d=screw_head); // 頭ザグリ
                }
        // ボード固定ネジ: 外側からザグリ + ボス内貫通 (先でキャリア穴へ締結)
        rotate([0,0,carrier_angle])
            for (h=mount_holes)
                translate([h[0], h[1], 0]) {
                    translate([0,0,-0.1])
                        cylinder(h=back_th+depth_inner-board_stack_th+0.2, d=board_screw_dia+0.5);
                    translate([0,0,-0.1]) cylinder(h=board_screw_head_h, d=board_screw_head);
                }
        // 通気/配線 (ボス位置を避ける)
        for (a=[0:60:359])
            rotate([0,0,a])
                translate([outer_dia/4,0,-0.1]) cylinder(h=back_th+0.2, d=3);
    }
}

// ============================================================================
//  スナップ試しクーポン (嵌合部だけをアークで切り出した小片)
//   実部品と同一ジオメトリから intersection で抜くので寸法は常に同期。
//   タレット(90)/コネクタ(0,20,-25)/ネジボス(45..)を避けた底側 180° を使用。
// ============================================================================
// 長尺クーポン: バネ爪を複数含めて連続した嵌合感と握りを得る。
//  ベゼル側が避けるのは turret(90)のみ → 底側へ。本体側は microSD(180,~186.6°)と
//  USB-C(270,~259.6°)の隙間=186.6〜259.6°(約73°)が連続クリーン、中央に boss(225)1個。
//  ctr223/deg68 でスリット(210/240)を含み、フィンガー225が完全フリー+前後が半フリー。
coupon_deg  = 68;                 // クーポンの角度幅 (清浄窓73°に収める)
coupon_ctr  = 223;                // 中心角 (microSD↔USB-C の隙間、boss225 を中央に)
coupon_ztop = bezel_face_th + 0.5;
coupon_zbot = -14;                // 嵌合部が入る高さまで

module sector(deg, r=200) {
    pts = concat([[0,0]], [for(a=[0:2:deg]) [r*cos(a), r*sin(a)]], [[0,0]]);
    linear_extrude(height=600, center=true) polygon(pts);
}
module coupon_clip() {
    intersection() {
        rotate([0,0, coupon_ctr - coupon_deg/2]) sector(coupon_deg);
        translate([0,0,(coupon_ztop+coupon_zbot)/2])
            cube([400, 400, coupon_ztop-coupon_zbot], center=true);
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
    } else if (part == "coupon_body") {
        intersection() { main_body();   coupon_clip(); }
    } else if (part == "coupon_bezel") {
        intersection() { front_bezel(); coupon_clip(); }
    } else if (part == "coupon") { // 嵌合確認用に両方を重ねて表示
        color("LightGray", 0.6) intersection() { main_body();   coupon_clip(); }
        color("SteelBlue", 0.8) intersection() { front_bezel(); coupon_clip(); }
    } else if (part == "coupon_button") {
        // 天面タレット周辺を実ジオメトリのまま切り出したボタン嵌合テスト片
        intersection() {
            main_body();
            rotate([0,0, button_angle])
                translate([(R_in + R_out + turret_protrude)/2, 0, button_z])
                    cube([(R_out+turret_protrude) - R_in + 20, 34, 30], center=true);
        }
    }
}

if (clip)
    intersection() { dispatch(); translate([-200,0,-200]) cube([400,200,400]); }
else
    dispatch();

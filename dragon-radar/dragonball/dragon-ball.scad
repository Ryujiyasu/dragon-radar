// ============================================================================
//  Dragon Ball — UWBタグ球体ケース (parametric)
//  本番: AnyCubic Photon M3 (レジン/MSLA, RESIONE かえんオレンジ水洗い)
//  プロト: Bambu A1 (FDM) でも可。星は印刷後に赤を差し塗り。
//  2半球分割: レジンはスライサが向き/サポートを付与。FDMは切断面下+ドーム頂部に軽サポート。
// ============================================================================
$fn = 160;

// ---- 主要パラメータ ----
ball_dia    = 70;    // 外径 [mm] (原作 ~75)
wall        = 2.2;   // 殻厚 (レジンは脆いので2mm以上)
star_count  = 1;     // 星の数 1..7 (DigiKey Make ONE Challenge → 一星球=1)
star_d      = 12;    // 星の外径(表面) [mm] (単独星なので大きめで映えさせる)
star_depth  = 1.0;   // 彫り込み深さ [mm]
star_spread = 9;     // 星クラスタの広がり(中心オフセット係数)
lip_h       = 4;     // 接合リップの差し込み高さ
lip_t       = 1.4;   // リップ肉厚
lip_gap     = 0.20;  // リップ嵌合クリア(片側)。レジンはタイトめ
part        = "both";// top / bottom / both / assembly / clip / bottom_print / top_print

// 排液/吸盤対策の穴 (ヘッドレス平置き印刷用。最終ボールは false のまま)
drain_holes      = false;
drain_hole_d     = 3.5;  // 排液穴径
drain_hole_n     = 4;    // 穴数
drain_apex_angle = 45;   // ドーム頂点からの角度(度)。星を避ける

R  = ball_dia/2;
Ri = R - wall;       // 内半径(空洞)

// ---- 5芒星 2D ----
module star2d(d) {
    ro = d/2; ri = d/2*0.40;
    polygon([ for (i=[0:9]) let(a=90 + i*36, r=(i%2==0 ? ro : ri)) [r*cos(a), r*sin(a)] ]);
}

// ---- 星の配置 (局所x,y, -1..1) N=1..7 サイコロ風=ドラゴンボール風 ----
function layout(n) =
    n<=1 ? [[0,0]] :
    n==2 ? [[-0.6,0],[0.6,0]] :
    n==3 ? [[0,0.7],[-0.6,-0.45],[0.6,-0.45]] :
    n==4 ? [[-0.6,0.6],[0.6,0.6],[-0.6,-0.6],[0.6,-0.6]] :
    n==5 ? [[-0.7,0.7],[0.7,0.7],[0,0],[-0.7,-0.7],[0.7,-0.7]] :
    n==6 ? [[-0.7,0.7],[0.7,0.7],[-0.7,0],[0.7,0],[-0.7,-0.7],[0.7,-0.7]] :
           [[-0.7,0.7],[0.7,0.7],[-0.7,0],[0.7,0],[-0.7,-0.7],[0.7,-0.7],[0,0]];

// ---- 星を上極(+Z)に彫る cutter ----
module star_debosses() {
    for (p = layout(star_count)) {
        x = p[0]*star_spread; y = p[1]*star_spread;
        zs = sqrt(max(R*R - x*x - y*y, 1));   // その(x,y)の球面z
        translate([x, y, zs - star_depth])
            linear_extrude(star_depth + 2) star2d(star_d);
    }
}

// ---- 中空球殻 + 星彫り ----
module shell() {
    difference() {
        sphere(R);
        sphere(Ri);
        star_debosses();
    }
}

// ---- 接合リップ (上半球に付け、下半球の空洞に差し込む) ----
//   connector(z>=0, 外径Ri=殻内壁とオーバーラップ) + insert(z<0, 外径Ri-gap=差込部)
module join_lip() {
    difference() {
        union() {
            cylinder(h = wall*2, r = Ri);                 // connector: 殻と一体化
            translate([0,0,-lip_h]) cylinder(h = lip_h, r = Ri - lip_gap); // insert
        }
        translate([0,0,-lip_h-0.1])
            cylinder(h = lip_h + wall*2 + 0.2, r = Ri - lip_gap - lip_t);  // 中空
    }
}

// 排液穴 (pole: +1=上半球頂点+Z / -1=下半球頂点-Z)。ドーム頂点から drain_apex_angle で穿孔
module drain_cuts(pole) {
    for (i = [0:drain_hole_n-1])
        rotate([0, 0, i*360/drain_hole_n + 45])
            rotate([0, pole > 0 ? drain_apex_angle : 180 - drain_apex_angle, 0])
                translate([0, 0, R])
                    cylinder(h = wall + 6, d = drain_hole_d, center = true);
}
module half_top() {   // 星のある側
    difference() {
        union() {
            intersection() { shell(); translate([0,0,-0.001]) cylinder(h=R+1, r=R+1); }
            join_lip();
        }
        if (drain_holes) drain_cuts(1);
    }
}
module half_bottom() {
    difference() {
        intersection() { shell(); translate([0,0,-R-1]) cylinder(h=R+1, r=R+1); }
        if (drain_holes) drain_cuts(-1);
    }
}

// ---- 出力 ----
if (part == "assembly") {
    half_bottom();
    color("orange", 0.55) half_top();
} else if (part == "top") {
    half_top();
} else if (part == "bottom") {
    half_bottom();
} else if (part == "bottom_print") {   // 平置き向き(切断面を下=ベッド、ドーム上)
    rotate([180,0,0]) half_bottom();
} else if (part == "top_print") {      // 上半球は自然向きで切断面が下
    half_top();
} else if (part == "clip") {        // 断面確認
    intersection() {
        union() { half_bottom(); half_top(); }
        translate([-200,0,-200]) cube([400,200,400]);
    }
} else {                            // both: 並べて表示
    translate([-R-4, 0, R]) rotate([180,0,0]) half_top();  // 切断面を下に
    translate([ R+4, 0, R])                    half_bottom();
}

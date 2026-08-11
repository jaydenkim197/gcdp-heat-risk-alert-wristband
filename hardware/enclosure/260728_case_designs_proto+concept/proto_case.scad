// ============================================================
// GCDP Heat Risk Alert Wristband — 프로토타입 케이스 (널널 버전 v2)
// 개발/디버깅 편의 최우선: 모든 모듈이 한 층에 나란히, 점퍼선 여유 확보
// v2: LTE 미확정 → 대형 예약 베이(26x28), 충전모듈(TP4056) 베이 추가
// 단위: mm  |  OpenSCAD (openscad.org)에서 열기
//
// 부품 근거 (260713 / 260718 문서):
//   ESP32-C3 SuperMini 22.5x18   | BE-220 GPS 22x20x6 (안테나 위)
//   배터리 TW-602035 20x35x6      | TMP117 25.4x18 | MAX30102 20.6x15.5
//   LTE Cat-M 모듈: 미확정 → 예약 베이 26x28 (M5Stamp/Waveshare급 다 수용)
//   충전모듈: TP4056 (26x17.5) 가정, USB 포트가 +X벽 바깥으로 노출
// ============================================================

// ---------- 출력 선택 ----------
part = "assembly";   // "base" | "lid" | "assembly" (STL 뽑을 땐 base/lid 각각)
show_components = true;   // 부품 고스트 표시 (STL 출력 시 자동 무시)
exploded = 12;            // assembly에서 뚜껑 띄우는 높이 (0 = 닫힘)

// ---------- 주요 파라미터 ----------
wall    = 2.5;    // 측벽 두께
floor_t = 2.0;    // 바닥 두께
lid_t   = 2.2;    // 뚜껑 두께
inner_x = 100;    // 내부 가로 (팔 방향)
inner_y = 70;     // 내부 세로 (손목 둘레 방향)
inner_z = 20;     // 내부 높이 — 점퍼선/커넥터 여유 (널널)
corner_r = 3;

outer_x = inner_x + 2*wall;   // = 105
outer_y = inner_y + 2*wall;   // = 75

strap_w = 26;     // 스트랩(벨크로) 폭 25mm + 여유

// ---------- 베이 위치 (내부좌표, 안쪽 바닥 좌하단 원점) ----------
// ESP32: 앞-왼쪽, USB-C는 -X벽 구멍으로
esp_pos  = [2, 10];   esp_sz  = [24, 19];
// MAX30102: 앞-중앙 (손목 압력이 안정적인 중앙부, 바닥 창)
max_pos  = [40, 4];   max_sz  = [21.5, 16.5];
// TMP117: 앞-오른쪽 (LTE 베이 대각선 반대 = 열 격리 최대)
tmp_pos  = [64, 3];   tmp_sz  = [26, 19.5];
// LTE 예약 베이: 뒤-왼쪽 (넉넉하게 26x28 — 웬만한 Cat-M 보드 다 들어감)
lte_pos  = [4, 32];   lte_sz  = [26, 28];
// 배터리: 뒤-중앙, 세로 방향 포켓
bat_pos  = [36, 32];  bat_sz  = [21, 36];  bat_wall_h = 8;
// GPS: 뒤-오른쪽, 위에 아무것도 없음 (세라믹 패치 하늘 방향)
gps_pos  = [62, 46];  gps_sz  = [23, 22];
// 충전모듈(TP4056): 중간-오른쪽, USB 포트가 +X벽 관통
//   ※ TMP117과 3~4mm 간격 — 충전 중에는 온도 측정값 무시할 것
chg_pos  = [74, 26];  chg_sz  = [26, 18.5];

// 나사 포스트 (M2 셀프태핑)
posts = [[4.5,4.5],[95.5,4.5],[4.5,65.5],[95.5,65.5]];

$fn = 48;

// ============================================================
// 유틸
// ============================================================
module rbox(x, y, z, r=corner_r) {
    linear_extrude(z)
        offset(r=r) offset(delta=-r)
            square([x, y]);
}
// 베이 울타리: 높이 3, 두께 1.2, 각 변 중앙에 배선 통로 6mm
module fence(pos, sz, h=3, t=1.2, gap=6) {
    translate([wall+pos[0], wall+pos[1], floor_t])
    difference() {
        linear_extrude(h) difference() {
            offset(delta=t) square(sz);
            square(sz);
        }
        // 배선 통로 4방향
        translate([sz[0]/2-gap/2, -t-0.1, -0.1]) cube([gap, sz[1]+2*t+0.2, h+0.2]);
        translate([-t-0.1, sz[1]/2-gap/2, -0.1]) cube([sz[0]+2*t+0.2, gap, h+0.2]);
    }
}
module ghost(pos, sz, h, c) {
    if (show_components)
        translate([wall+pos[0], wall+pos[1], floor_t])
            %color(c, 0.55) cube([sz[0], sz[1], h]);
}

// ============================================================
// 본체 (base)
// ============================================================
module base() {
    difference() {
        union() {
            rbox(outer_x, outer_y, floor_t + inner_z);
            // 스트랩 루프 (±Y 장벽 바깥, 하단) — 밴드가 손목을 감는 방향
            for (yy = [-6, outer_y])
                translate([outer_x/2 - strap_w/2 - 4, yy, 0])
                    strap_loop();
            // MAX30102 차광 림 (케이스 바닥 바깥쪽, 검정 필라멘트 권장)
            translate([wall+max_pos[0]+max_sz[0]/2, wall+max_pos[1]+max_sz[1]/2, -1.5])
                linear_extrude(1.5) difference() {
                    offset(r=2) square([9+4, 7+4], center=true);
                    square([9+1, 7+1], center=true);
                }
        }
        // 내부 공동
        translate([wall, wall, floor_t]) cube([inner_x, inner_y, inner_z+1]);
        // MAX30102 광학 창 (9x7 관통)
        translate([wall+max_pos[0]+max_sz[0]/2, wall+max_pos[1]+max_sz[1]/2, -2])
            linear_extrude(floor_t+4) square([9, 7], center=true);
        // TMP117 열전달 박막 (바닥을 0.8mm만 남기고 아래서 제거)
        translate([wall+tmp_pos[0]+tmp_sz[0]/2, wall+tmp_pos[1]+tmp_sz[1]/2, -0.1])
            linear_extrude(floor_t-0.8+0.1) square([14, 10], center=true);
        // ESP32 USB-C 구멍 (-X벽)
        translate([-0.1, wall+esp_pos[1]+esp_sz[1]/2-5, floor_t+1])
            cube([wall+0.2, 10, 6]);
        // 충전모듈 USB 구멍 (+X벽) — 케이스 안 열고 충전
        translate([wall+inner_x-0.1, wall+chg_pos[1]+chg_sz[1]/2-6, floor_t+1])
            cube([wall+0.2, 12, 6]);
        // 나사 파일럿 구멍
        for (p = posts)
            translate([wall+p[0], wall+p[1], floor_t+2])
                cylinder(d=1.8, h=inner_z);
    }
    // 나사 포스트
    for (p = posts)
        translate([wall+p[0], wall+p[1], floor_t])
            difference() {
                cylinder(d=7, h=inner_z);
                translate([0,0,2]) cylinder(d=1.8, h=inner_z);
            }
    // 배터리 포켓 (전용 벽, 위 개방)
    translate([wall+bat_pos[0], wall+bat_pos[1], floor_t])
        difference() {
            linear_extrude(bat_wall_h) difference() {
                offset(delta=1.5) square(bat_sz);
                square(bat_sz);
            }
            // 배선 노치 (앞쪽 벽) — 충전모듈 방향
            translate([bat_sz[0]/2-4, -1.6, bat_wall_h-4]) cube([8, 2, 5]);
        }
    // 모듈 울타리
    fence(esp_pos, esp_sz);
    fence(max_pos, max_sz);
    fence(tmp_pos, tmp_sz);
    fence(lte_pos, lte_sz);
    fence(gps_pos, gps_sz);
    fence(chg_pos, chg_sz);
}
module strap_loop() {
    // 26mm 스트랩용 루프 바
    w = strap_w + 8;
    difference() {
        cube([w, 6, 9]);
        translate([4, -0.1, 2.5]) cube([w-8, 3.6, 4]);
    }
}

// ============================================================
// 뚜껑 (lid)
// ============================================================
module lid() {
    difference() {
        union() {
            rbox(outer_x, outer_y, lid_t);
            // 안쪽 립 (본체 공동에 삽입)
            translate([wall+0.25, wall+0.25, -2])
                rbox(inner_x-0.5, inner_y-0.5, 2, r=2);
        }
        // 나사 관통 + 카운터싱크
        for (p = posts) translate([wall+p[0], wall+p[1], -2.5]) {
            cylinder(d=2.4, h=lid_t+5);
            translate([0,0,2+lid_t-1]) cylinder(d1=2.4, d2=4.6, h=1.1);
        }
        // 트리거 버튼 구멍 (GPIO4, MAX 창과 배터리 포켓 사이 빈 공간 위)
        translate([wall+50, wall+27, -3]) cylinder(d=7.5, h=8);
        // 리셋 접근 구멍 (ESP 위)
        translate([wall+14, wall+25, -3]) cylinder(d=3, h=8);
        // 부저 사운드홀 (LTE 베이 위 뚜껑 안쪽에 부저 접착 — 공간 여유 12mm)
        for (a = [0:60:300])
            translate([wall+17+6*cos(a), wall+46+6*sin(a), -3]) cylinder(d=1.6, h=8);
        translate([wall+17, wall+46, -3]) cylinder(d=1.6, h=8);
        // 라벨 각인
        translate([outer_x/2, wall+8, lid_t-0.6])
            linear_extrude(0.7) text("GCDP HEAT-ALERT PROTO", size=4.2,
                halign="center", font="Arial:style=Bold");
    }
}

// ============================================================
// 부품 고스트 + 출력
// ============================================================
module components() {
    ghost(esp_pos, esp_sz, 5,  "royalblue");  // ESP32-C3
    ghost(max_pos, max_sz, 3,  "red");        // MAX30102
    ghost(tmp_pos, tmp_sz, 3,  "orange");     // TMP117
    ghost(lte_pos, lte_sz, 8,  "purple");     // LTE 예약 (넉넉)
    ghost(bat_pos, bat_sz, 6,  "gray");       // 배터리
    ghost(gps_pos, gps_sz, 6,  "green");      // BE-220 GPS
    ghost(chg_pos, chg_sz, 5,  "hotpink");    // TP4056 충전모듈
}

if (part == "base")  base();
if (part == "lid")   lid();
if (part == "assembly") {
    base();
    components();
    translate([0, 0, floor_t + inner_z + 2 + exploded]) lid();
}

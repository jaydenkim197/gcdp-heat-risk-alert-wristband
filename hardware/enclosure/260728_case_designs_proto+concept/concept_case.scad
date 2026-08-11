// ============================================================
// GCDP Heat Risk Alert Wristband — 컨셉 케이스 (이상적 최종 버전)
// 260713 문서 "Later final version" 방향: 통합 PCB 전제, 스마트워치 폼팩터
// 수직 스택: 피부면 센서 → 통합 PCB(ESP32+LTE) → 배터리 → GPS(최상층)
// 단위: mm  |  OpenSCAD에서 열기
// ============================================================

// ---------- 출력 선택 ----------
part = "assembly";      // "shell" | "bottom" | "assembly"
show_components = true; // 내부 스택 고스트
exploded = 0;           // 바닥 플레이트 분리 거리 (0 = 조립)

// ---------- 주요 파라미터 ----------
body_x  = 48;      // 팔 방향
body_y  = 44;      // 손목 둘레 방향
body_h  = 16;      // 전체 높이 (통합 설계 목표 14~17)
body_r  = 11;      // 모서리 라운드
wall    = 2.0;
bot_t   = 1.4;     // 바닥(센서) 플레이트 두께
wrist_R = 45;      // 손목 곡률 반경 (곡면 밑면)
band_w  = 24;      // 시계줄 폭
lug_gap = 3;       // 러그-밴드 간격

$fn = 72;

// ============================================================
// 유틸
// ============================================================
module rbody(x, y, z, r) {
    linear_extrude(z) offset(r=r) offset(delta=-r)
        square([x, y], center=true);
}
// 손목 곡면 절단용 실린더 (팔 방향 = X축)
module wrist_cut() {
    translate([0, 0, -wrist_R + 2.5])
        rotate([0, 90, 0]) cylinder(r=wrist_R, h=body_x+40, center=true);
}

// ============================================================
// 메인 쉘 (윗몸통) — 위에서 씌우는 구조
// ============================================================
module shell() {
    difference() {
        union() {
            // 몸통 (위로 갈수록 살짝 좁아지는 2단 프로파일)
            hull() {
                rbody(body_x, body_y, 1, body_r);
                translate([0,0,body_h-2]) rbody(body_x-3, body_y-3, 2, body_r-1);
            }
            // 러그 (±Y, 24mm 밴드)
            for (s = [1, -1]) lug(s);
        }
        // 손목 곡면 (밑면을 오목하게)
        wrist_cut();
        // 내부 공동 (상판 2.5 남김)
        translate([0, 0, bot_t])
            rbody(body_x-2*wall, body_y-2*wall, body_h-2.5-bot_t, body_r-wall);
        // 바닥 플레이트 안착 립
        translate([0, 0, -6])
            rbody(body_x-2*wall+1.2, body_y-2*wall+1.2, 6+bot_t+0.05, body_r-wall);
        // GPS 스카이라이트 (상판을 0.8mm만 남김 — 세라믹 패치 바로 위)
        translate([14, 0, body_h-2.5-0.01])
            rbody(24, 22, 2.5-0.8, 4);
        // 트리거 버튼 리세스 (오목하게 넣어 오작동 방지) + 관통
        translate([15, -12, body_h-3.5]) cylinder(d=10, h=5);
        translate([15, -12, body_h-8])   cylinder(d=6.2, h=8);
        // USB-C 포트 (+X 측면, 스트랩에 가려지지 않는 면)
        translate([body_x/2-wall-1, -5, 4.5]) cube([wall+2, 10, 3.4]);
        // 부저 마이크로 슬릿 (-X 측면 3줄)
        for (i = [-1:1])
            translate([-body_x/2-1, -3+i*4, 9]) cube([wall+2, 1.2, 6]);
    }
}
module lug(s) {
    // 일체형 러그 + 스프링바 구멍
    for (x = [-band_w/2-2.5, band_w/2+2.5])
        translate([x, s*(body_y/2+lug_gap/2), 0])
            difference() {
                hull() {
                    translate([0, -s*lug_gap, 2]) cube([5, 1, 8], center=true);
                    translate([0, s*1.5, 3.5]) rotate([0,90,0])
                        cylinder(d=6, h=5, center=true);
                }
                translate([0, s*1.5, 3.5]) rotate([90,0,90])
                    cylinder(d=1.4, h=7, center=true);
                wrist_cut();
            }
    // 스프링바 (표시용)
    if (show_components)
        translate([0, s*(body_y/2+lug_gap/2+1.5), 3.5])
            %color("silver") rotate([0,90,0]) cylinder(d=1.3, h=band_w+8, center=true);
}

// ============================================================
// 바닥 센서 플레이트 (피부 접촉면) — 스냅 삽입
// ============================================================
module bottom() {
    difference() {
        union() {
            // 곡면 따라가는 플레이트
            intersection() {
                translate([0,0,-10]) rbody(body_x-2*wall+1, body_y-2*wall+1, 10+bot_t, body_r-wall);
                difference() {
                    translate([0,0,-30]) rbody(body_x, body_y, 40, body_r);
                    wrist_cut();
                }
            }
            // 센서 포드 (중앙 1.2mm 돌출 → 피부 밀착 압력 집중)
            translate([0, -2, 0]) sensor_pod();
        }
        // MAX30102 광학 창 (8x6)
        translate([-6, -2, -12]) linear_extrude(15) square([8, 6], center=true);
        // TMP117 열섬 박막 (0.6mm)
        translate([10, -2, -12+0.01]) linear_extrude(12-0.6) square([10, 8], center=true);
    }
}
module sensor_pod() {
    difference() {
        hull() {
            translate([0,0,-1.2]) linear_extrude(0.5) offset(r=3) square([26, 14], center=true);
            linear_extrude(0.5) offset(r=5) square([28, 16], center=true);
        }
        // 광학 창 주변 차광 링 홈 (검정 실리콘/TPU 인서트 자리)
        translate([-6, 0, -1.3]) linear_extrude(0.7) difference() {
            offset(r=1.5) square([9, 7], center=true);
            square([9, 7], center=true);
        }
    }
}

// ============================================================
// 내부 스택 고스트 (이상적 통합 구성)
// ============================================================
module components() {
    if (show_components) {
        // 1층: 센서 (바닥 플레이트 위)
        %color("red", 0.6)    translate([-6, -4, 0.3])  cube([12, 12, 1.5], center=false);  // MAX30102 (PCB 직실장)
        %color("orange", 0.6) translate([6, -5, 0.3])   cube([6, 6, 1.2]);                  // TMP117 (베어칩)
        // 2층: 통합 메인 PCB (ESP32-C3 + LTE Cat-M 온보드)
        %color("royalblue", 0.55) translate([0, 0, 2.6])  rbody(38, 33, 1.6, 6);
        // 3층: 배터리(세로 배치)와 GPS를 PCB 위에 나란히
        %color("gray", 0.6)   translate([-19, -17.5, 4.4]) cube([20, 35, 6]);   // 배터리 380mAh
        %color("green", 0.6)  translate([3, -10, 4.4])     cube([22, 20, 6]);   // BE-220 GPS (스카이라이트 아래)
    }
}

if (part == "shell")  shell();
if (part == "bottom") bottom();
if (part == "assembly") {
    shell();
    translate([0, 0, -exploded]) bottom();
    components();
}

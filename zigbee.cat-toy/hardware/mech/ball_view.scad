// Previews of the ball parts. Run from build/ball_<cell>/ (ball.sh copies this file there):
//   openscad -o x.png -D 'part="exploded"' ball_view.scad
// parts: exploded | chassis | core | core_open | core_top | section | joint | closed
part = "exploded";
module chassis() {
  color("SteelBlue") import("frame.stl");
  color("DarkSeaGreen") import("tray.stl");
  color("Gold") import("battery.stl");
  color("DimGray") import("motor.stl");
  color("DarkOliveGreen") import("board_full.stl");
  color("Silver") import("ballast.stl");
}
module core_cup() { color("LightSlateGray", 0.95) import("core_cup.stl"); }
module core_cap() { color("Gainsboro", 0.95) import("core_cap.stl"); }
module shell() {
  color("Coral") import("dome_a.stl");
  color("OrangeRed") import("ring.stl");
  color("Coral") import("dome_b.stl");
}
if (part == "chassis") chassis();
if (part == "core") { chassis(); core_cup(); core_cap(); }
if (part == "core_open") { chassis(); core_cup(); translate([0,0,25]) core_cap(); }
if (part == "core_top") { core_cup(); core_cap(); }
if (part == "exploded") {
  translate([0,0,-45]) color("Coral", 0.9) import("dome_a.stl");
  translate([0,0,-10]) core_cup();
  chassis();
  translate([0,0,25]) core_cap();
  translate([0,0,45]) color("OrangeRed") import("ring.stl");
  translate([0,0,75]) color("Coral", 0.9) import("dome_b.stl");
}
if (part == "closed") shell();
module half(c) { intersection() { color(c) children(); color(c) translate([-200,-100,-100]) cube([200,200,200]); } }   // keep X<0
module sect() {
  half("Coral") import("dome_a.stl");
  half("OrangeRed") import("ring.stl");
  half("Coral") import("dome_b.stl");
  half("LightSlateGray") import("core_cup.stl");
  half("Gainsboro") import("core_cap.stl");
  if ($children == 0) {
    half("SteelBlue") import("frame.stl");
    half("DarkSeaGreen") import("tray.stl");
    half("Gold") import("battery.stl");
    half("DimGray") import("motor.stl");
    half("DarkOliveGreen") import("board_full.stl");
    half("Silver") import("ballast.stl");
  }
}
if (part == "section") sect();
if (part == "joint") sect() { cube(0); }   // shell + core only; view it with --camera=0,36,0,90,0,90,70

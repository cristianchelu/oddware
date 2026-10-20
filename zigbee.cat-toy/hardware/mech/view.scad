part = "cradle";
if (part == "cradle") color("SteelBlue") import("cradle.stl");
if (part == "lid") color("LightGray") import("lid.stl");
if (part == "box") color("DarkSeaGreen") import("battery_box.stl");
if (part == "assembly") { color("SteelBlue") import("cradle.stl"); color("DarkSeaGreen") import("battery_box.stl"); %color("LightGray",0.4) translate([0,0,6]) import("lid.stl"); }

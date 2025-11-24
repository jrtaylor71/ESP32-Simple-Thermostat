// ESP32 Simple Thermostat 3D Printable Case
// Adapted from Honeywell UWP style mounting
// Wall-mounted design with front bezel and back plate
// License: GPL v3

// ===== USER ADJUSTABLE PARAMETERS =====

// PCB dimensions
pcb_width = 100;           // Width of PCB in mm
pcb_height = 80;           // Height of PCB in mm
pcb_thickness = 1.6;       // Standard PCB thickness

// Display dimensions (3.2" TFT)
display_width = 76.5;      // Active display area width
display_height = 51;       // Active display area height
display_bezel = 2;         // Bezel around display

// Case dimensions
wall_thickness = 2.5;      // Case wall thickness
case_depth_front = 12;     // Front bezel depth
case_depth_back = 20;      // Back plate depth
corner_radius = 3;         // Rounded corners
pcb_clearance = 2;         // Space between PCB and walls

// Mounting hardware
pcb_mount_dia = 2.5;       // M2 screw holes
pcb_mount_depth = 4;       // Heat insert depth
case_couple_dia = 3.5;     // M3 coupling screws between front/back
wall_mount_dia = 5;        // M5 wall mounting holes
standoff_height = 15;      // Standoff height from back plate
standoff_dia = 8;          // Standoff outer diameter

// Cable management
cable_hole_dia = 18;       // Hole for wiring

// ===== CALCULATED DIMENSIONS =====

case_width = pcb_width + (pcb_clearance * 2) + (wall_thickness * 2);
case_height = pcb_height + (pcb_clearance * 2) + (wall_thickness * 2);

// ===== MODULES =====

// Rounded rectangle (2D)
module rounded_rect(w, h, r) {
    hull() {
        translate([r, r, 0]) circle(r=r, $fn=32);
        translate([w-r, r, 0]) circle(r=r, $fn=32);
        translate([r, h-r, 0]) circle(r=r, $fn=32);
        translate([w-r, h-r, 0]) circle(r=r, $fn=32);
    }
}

// Heat insert pocket (for M2)
module heat_insert_pocket(dia, depth) {
    cylinder(h=depth, r=dia/2.2, $fn=24);
}

// Standoff with M2 screw hole
module standoff(height, hole_dia, outer_dia) {
    difference() {
        cylinder(h=height, r=outer_dia/2, $fn=32);
        translate([0, 0, -1])
            cylinder(h=height+2, r=hole_dia/2, $fn=24);
    }
}

// ===== FRONT BEZEL - Display opening and PCB retainer =====
module case_front() {
    difference() {
        // Main front housing
        linear_extrude(height=case_depth_front)
            rounded_rect(case_width, case_height, corner_radius);
        
        // Display opening
        display_x = (case_width - display_width) / 2;
        display_y = (case_height - display_height) / 2;
        translate([display_x, display_y, -1])
            cube([display_width, display_height, case_depth_front + 2]);
        
        // Interior pocket for PCB
        translate([wall_thickness, wall_thickness, wall_thickness])
            cube([case_width - wall_thickness*2, 
                  case_height - wall_thickness*2, 
                  case_depth_front - wall_thickness + 1]);
        
        // Ventilation holes all around the perimeter (small holes)
        vent_dia = 3;
        vent_spacing = 8;
        
        // Top ventilation
        for (i = [0:floor((case_width - 20)/vent_spacing)]) {
            translate([10 + i*vent_spacing, case_height - 3, case_depth_front/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Bottom ventilation
        for (i = [0:floor((case_width - 20)/vent_spacing)]) {
            translate([10 + i*vent_spacing, 3, case_depth_front/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Left side ventilation
        for (i = [0:floor((case_height - 20)/vent_spacing)]) {
            translate([3, 10 + i*vent_spacing, case_depth_front/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Right side ventilation
        for (i = [0:floor((case_height - 20)/vent_spacing)]) {
            translate([case_width - 3, 10 + i*vent_spacing, case_depth_front/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Coupling screw holes (M3) - top and bottom
        translate([case_width/2, 5, case_depth_front/2])
            cylinder(h=case_depth_front+2, r=case_couple_dia/2, $fn=24);
        translate([case_width/2, case_height - 5, case_depth_front/2])
            cylinder(h=case_depth_front+2, r=case_couple_dia/2, $fn=24);
    }
    
    // Inner lips to help position back piece
    lip_depth = 2;
    lip_height = 1.5;
    translate([wall_thickness + 1, wall_thickness + 1, case_depth_front - lip_depth])
        cube([case_width - wall_thickness*2 - 2, 
              case_height - wall_thickness*2 - 2, 
              lip_height]);
}

// ===== BACK PLATE - Wall mounting with standoffs =====
module case_back() {
    // 4 Wall mounting holes (M5) at corners - moved inward
    mount_offset = 15;  // Moved from 10 to 15
    mount_positions = [
        [mount_offset, mount_offset],
        [case_width - mount_offset, mount_offset],
        [mount_offset, case_height - mount_offset],
        [case_width - mount_offset, case_height - mount_offset]
    ];
    
    // Standoff positions for PCB mounting (M2 with heat inserts)
    standoff_positions = [
        [wall_thickness + 5, wall_thickness + 5],
        [case_width - wall_thickness - 5, wall_thickness + 5],
        [wall_thickness + 5, case_height - wall_thickness - 5],
        [case_width - wall_thickness - 5, case_height - wall_thickness - 5]
    ];
    
    difference() {
        // Main back housing
        linear_extrude(height=case_depth_back)
            rounded_rect(case_width, case_height, corner_radius);
        
        // Interior pocket
        translate([wall_thickness, wall_thickness, wall_thickness])
            cube([case_width - wall_thickness*2, 
                  case_height - wall_thickness*2, 
                  case_depth_back - wall_thickness + 1]);
        
        // 4 Wall mounting holes (M5) at corners
        for (pos = mount_positions) {
            translate([pos[0], pos[1], -1])
                cylinder(h=case_depth_back+2, d=wall_mount_dia, $fn=32);
        }
        
        // Cable hole at back (larger for wires)
        translate([case_width/2, case_height/2, -1])
            cylinder(h=case_depth_back+2, d=25, $fn=32);
        
        // Ventilation holes all around the perimeter (small holes)
        vent_dia = 3;
        vent_spacing = 8;
        
        // Top ventilation
        for (i = [0:floor((case_width - 20)/vent_spacing)]) {
            translate([10 + i*vent_spacing, case_height - 3, case_depth_back/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Bottom ventilation
        for (i = [0:floor((case_width - 20)/vent_spacing)]) {
            translate([10 + i*vent_spacing, 3, case_depth_back/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Left side ventilation
        for (i = [0:floor((case_height - 20)/vent_spacing)]) {
            translate([3, 10 + i*vent_spacing, case_depth_back/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Right side ventilation
        for (i = [0:floor((case_height - 20)/vent_spacing)]) {
            translate([case_width - 3, 10 + i*vent_spacing, case_depth_back/2])
                cylinder(h=wall_thickness+2, d=vent_dia, $fn=24);
        }
        
        // Coupling screw holes (M3) - top and bottom
        translate([case_width/2, 5, case_depth_back/2])
            cylinder(h=case_depth_back+2, r=case_couple_dia/2, $fn=24);
        translate([case_width/2, case_height - 5, case_depth_back/2])
            cylinder(h=case_depth_back+2, r=case_couple_dia/2, $fn=24);
    }
    
    // Standoffs for PCB mounting (M2 with heat inserts)
    for (pos = standoff_positions) {
        translate([pos[0], pos[1], wall_thickness])
            standoff(standoff_height, pcb_mount_dia, standoff_dia);
    }
    
    // Wall mounting tabs (optional - for friction fit on drywall anchors)
    tab_height = 3;
    tab_width = 4;
    tab_depth = 2;
    
    for (pos = mount_positions) {
        translate([pos[0] - tab_width/2, pos[1] - tab_width/2, case_depth_back])
            cube([tab_width, tab_width, tab_depth]);
    }
}

// ===== RENDER SELECTION =====

// Set render_part to:
// "front" - Front bezel with display opening
// "back" - Back plate with wall mounting holes
// "both" - Both parts assembled (for visualization)

render_part = "both"; // Change this to render different parts

if (render_part == "front") {
    case_front();
} else if (render_part == "back") {
    case_back();
} else if (render_part == "both") {
    case_back();
    translate([0, 0, case_depth_back + case_depth_front])
        case_front();
}

// ===== ASSEMBLY INSTRUCTIONS =====
// 
// PRINTING:
// - Print case_front.stl with supports under display bezel
// - Print case_back.stl with supports for wall mount tabs
// - Use 0.4mm nozzle, 0.2mm layer height, 20% infill
//
// HARDWARE NEEDED:
// - 4x M5 wall anchors/bolts for wall mounting
// - 3x M2 screws (10mm) for PCB mounting
// - 3x M2 heat inserts for PCB mounting
// - 2x M3 screws (12mm) for coupling front/back
// - 2x M3 heat inserts for coupling
// - Wires for display and sensor connections
//
// ASSEMBLY:
// 1. Install M2 heat inserts in standoffs on back piece
// 2. Mount back piece to wall using M5 bolts through corner holes
// 3. Mount PCB to standoffs using M2 screws with washers
// 4. Route display and sensor cables through cable hole
// 5. Install M3 heat inserts in front piece coupling holes
// 6. Align front piece with back piece and secure with M3 screws

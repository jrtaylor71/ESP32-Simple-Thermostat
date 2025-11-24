// ESP32 Simple Thermostat 3D Printable Case - FRONT BEZEL
// Adapted from Honeywell UWP style mounting
// Display opening and PCB retainer
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
corner_radius = 3;         // Rounded corners
pcb_clearance = 2;         // Space between PCB and walls

// Mounting hardware
case_couple_dia = 3.5;     // M3 coupling screws between front/back

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

// ===== RENDER =====
case_front();

// ===== ASSEMBLY INSTRUCTIONS =====
// 
// PRINTING:
// - Print with supports under display bezel
// - Use 0.4mm nozzle, 0.2mm layer height, 20% infill
//
// HARDWARE NEEDED (for front piece):
// - 2x M3 screws (12mm) for coupling to back
// - 2x M3 heat inserts for coupling
//
// ASSEMBLY:
// 1. Install M3 heat inserts in coupling screw holes
// 2. After back piece is mounted to wall with PCB installed,
//    align front piece with back piece and secure with M3 screws

# 3D Printable Case for ESP32 Simple Thermostat

This directory contains the 3D printable case design files for the ESP32 Simple Thermostat project.

## Files

- `thermostat_case.scad` - OpenSCAD parametric design file (editable)
- `case_back.stl` - STL file for the back case (ready to print)
- `case_front.stl` - STL file for the front bezel (ready to print)

## Design Features

### Back Case
- Wall-mountable with keyhole mounting system
- PCB mounting posts with M3 screw holes
- Ventilation slots on both sides for relay cooling
- Cable management hole for HVAC wiring (20mm diameter)
- USB port access for programming
- Rounded corners for professional appearance

### Front Bezel
- Display cutout for 3.2" TFT touchscreen
- Bezel frame around display
- Snap-fit or screw attachment to back case
- Smooth surface for easy cleaning

## Customization

The OpenSCAD file (`thermostat_case.scad`) includes adjustable parameters at the top of the file:

```openscad
// PCB dimensions
pcb_width = 100;           // Adjust to match your PCB
pcb_height = 80;           // Adjust to match your PCB

// Display dimensions
display_width = 76.5;      // 3.2" TFT active area
display_height = 51;

// Case dimensions
wall_thickness = 2.5;
case_depth = 35;           // Increase if components don't fit
```

**Before printing**, you should:
1. Measure your actual PCB dimensions
2. Verify mounting hole positions
3. Check display offset from PCB edge
4. Adjust `case_depth` if needed for tall components

## Printing Instructions

### Recommended Settings
- **Material**: PLA or PETG
- **Layer Height**: 0.2mm (0.15mm for better finish)
- **Infill**: 20%
- **Wall Lines**: 3-4
- **Top/Bottom Layers**: 4-5
- **Supports**: Only for USB port cutout (if needed)
- **Build Plate Adhesion**: Brim recommended

### Print Orientation
- **Back Case**: Print with mounting holes facing up (flat side down)
- **Front Bezel**: Print face-down for smooth surface finish

### Post-Processing
1. Remove any support material
2. Test fit PCB before final assembly
3. Clean screw holes with a 3mm drill bit if needed
4. Sand display bezel edges for smooth finish

## Assembly Instructions

### Required Hardware
- 4x M3 x 8mm screws (PCB mounting)
- 4x M3 x 10mm screws (front bezel attachment)
- 2x #6 or #8 drywall screws (wall mounting)

### Assembly Steps

1. **Install PCB in Back Case**
   - Insert PCB through the front opening
   - Align with mounting posts
   - Secure with M3 x 8mm screws

2. **Route Cables**
   - Feed HVAC wiring through bottom cable hole
   - Route USB cable through side port (if needed)
   - Organize wires to avoid display interference

3. **Connect Display**
   - Carefully connect display ribbon cable
   - Ensure display sits flat against PCB

4. **Attach Front Bezel**
   - Align display with cutout
   - Press bezel onto case (snap fit)
   - OR secure with M3 x 10mm screws at corners

5. **Wall Mounting**
   - Install drywall screws in wall (use anchors if needed)
   - Leave screw heads ~3mm proud of wall
   - Hang case using keyhole slots
   - Slide down to lock in place

## Modifications

### To Add Features

**Larger Cable Hole**: Edit line in OpenSCAD:
```openscad
cable_hole_dia = 25;  // Increase from 20mm
```

**Different Display Size**: Measure your display and update:
```openscad
display_width = XX;   // Your display width
display_height = XX;  // Your display height
```

**Deeper Case**: If components don't fit:
```openscad
case_depth = 40;  // Increase from 35mm
```

### Generating STL Files

1. Open `thermostat_case.scad` in OpenSCAD
2. Set `render_part = "back"` at bottom of file
3. Press F6 to render (may take a minute)
4. File → Export → Export as STL → Save as `case_back.stl`
5. Set `render_part = "front"`
6. Press F6 to render
7. File → Export → Export as STL → Save as `case_front.stl`

## Troubleshooting

### PCB Doesn't Fit
- Increase `pcb_clearance` parameter
- Verify PCB dimensions are correct
- Check for components extending beyond PCB edge

### Display Not Centered
- Measure display offset from PCB edge
- Adjust `display_offset_x` and `display_offset_y`

### Screw Holes Don't Align
- Measure actual mounting hole positions on your PCB
- Edit `mounting_positions` array in OpenSCAD
- Regenerate STL files

### Case Too Deep/Shallow
- Measure tallest component height
- Adjust `case_depth` parameter
- Add extra 3-5mm clearance

## License

This case design is released under GPL v3, matching the main project license.

## Credits

Designed for the ESP32 Simple Thermostat project by jrtaylor71.

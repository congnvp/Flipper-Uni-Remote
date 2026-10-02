#include "remote.h"

bool uni_element_focusable(const UniElement* element) {
    if(!element) return false;
    return element->type == UniElementButton || element->type == UniElementHStep ||
           element->type == UniElementVStep || element->type == UniElementDpad;
}

const char* uni_transport_label(UniTransport transport) {
    switch(transport) {
    case UniTransportInfrared:
        return "IR";
    case UniTransportBluetoothHid:
        return "BT";
    case UniTransportStatefulIr:
        return "IR";
    default:
        return "--";
    }
}

const char* uni_element_type_name(UniElementType type) {
    switch(type) {
    case UniElementStatus:
        return "status";
    case UniElementScreen:
        return "screen";
    case UniElementButton:
        return "button";
    case UniElementHStep:
        return "hstep";
    case UniElementVStep:
        return "vstep";
    case UniElementDpad:
        return "dpad";
    default:
        return "button";
    }
}


bool uni_element_occupies_cell_at(
    const UniElement* element,
    uint8_t anchor_x,
    uint8_t anchor_y,
    uint8_t cell_x,
    uint8_t cell_y) {
    if(!element) return false;
    if(cell_x < anchor_x || cell_y < anchor_y) return false;

    const uint8_t local_x = cell_x - anchor_x;
    const uint8_t local_y = cell_y - anchor_y;
    if(local_x >= element->w || local_y >= element->h) return false;

    /*
     * D-pad uses a cross-shaped occupancy mask. Its bounding Rect remains 3x3
     * for rendering and text compatibility, but the four corner cells are real
     * free layout slots.
     */
    if(element->type == UniElementDpad && element->w >= 3 && element->h >= 3) {
        return local_x == element->w / 2 || local_y == element->h / 2;
    }

    return true;
}

bool uni_element_occupies_cell(const UniElement* element, uint8_t cell_x, uint8_t cell_y) {
    if(!element) return false;
    return uni_element_occupies_cell_at(
        element,
        element->x,
        element->y,
        cell_x,
        cell_y);
}

int uni_element_direction_score(
    const UniElement* from,
    const UniElement* to,
    int8_t dx,
    int8_t dy) {
    if(!from || !to) return -1;
    if((dx == 0 && dy == 0) || (dx != 0 && dy != 0)) return -1;

    int best = -1;
    for(uint8_t fy = 0; fy < 6; fy++) {
        for(uint8_t fx = 0; fx < 3; fx++) {
            if(!uni_element_occupies_cell(from, fx, fy)) continue;

            for(uint8_t ty = 0; ty < 6; ty++) {
                for(uint8_t tx = 0; tx < 3; tx++) {
                    if(!uni_element_occupies_cell(to, tx, ty)) continue;

                    int primary = 0;
                    int secondary = 0;
                    if(dx > 0 && tx > fx) {
                        primary = tx - fx;
                        secondary = ty > fy ? ty - fy : fy - ty;
                    } else if(dx < 0 && tx < fx) {
                        primary = fx - tx;
                        secondary = ty > fy ? ty - fy : fy - ty;
                    } else if(dy > 0 && ty > fy) {
                        primary = ty - fy;
                        secondary = tx > fx ? tx - fx : fx - tx;
                    } else if(dy < 0 && ty < fy) {
                        primary = fy - ty;
                        secondary = tx > fx ? tx - fx : fx - tx;
                    } else {
                        continue;
                    }

                    /* Prefer same row/column first, then nearest distance. */
                    const int score = secondary * 16 + primary;
                    if(best < 0 || score < best) best = score;
                }
            }
        }
    }
    return best;
}

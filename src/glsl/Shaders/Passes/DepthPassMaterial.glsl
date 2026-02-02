#include "surface.glsl"


void pass_main(Surface surface) {
	if (surface.shouldDiscard) {
		discard;
	}
}
const int TONEMAP_REINHARD = 0;
const int TONEMAP_RRT = 1;
const int TONEMAP_FILMIC = 2;
const int TONEMAP_ACES = 3;

vec3 Reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 Filmic(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 gammaCorrect(vec3 color) {
    const float gamma = 2.2;
    return pow(color, vec3(1.0 / gamma));
}

vec3 Tonemap(vec3 inColor, int tonemapType, float exposure) {
    vec3 color = inColor * exposure;

    vec3 tonemapped;
    switch (tonemapType) {
        case TONEMAP_REINHARD: {
            tonemapped = Reinhard(color);
            break;
        }
        case TONEMAP_RRT: {
            tonemapped = RRTAndODTFit(color);
            break;
        }
        case TONEMAP_FILMIC: {
            tonemapped = Filmic(color);
            break;
        }
        case TONEMAP_ACES: {
            tonemapped = ACES(color);
            break;
        }
        default: tonemapped = vec3(0);
    }

    vec3 gammaCorrected = gammaCorrect(tonemapped);

    return gammaCorrected;
}
function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end
vec2 meshGetScreenPos(vec4 hPos)
{
  return ((hPos.xy / hPos.w) * 0.5 + 0.5) * view.viewportf.xy;
}

bool isFrontFacingHW(vec4 ha, vec4 hb, vec4 hc)
{
  return determinant(mat3(ha.xyw, hb.xyw, hc.xyw)) <= 0.0;
}

void meshPixelBboxEpsilon(inout vec2 pixelMin, inout vec2 pixelMax)
{
  const float epsilon = 1.0 / 256.0;
  pixelMin            = round(pixelMin - epsilon);
  pixelMax            = round(pixelMax + epsilon);
}

bool meshPixelBboxCull(vec2 pixelMin, vec2 pixelMax)
{
  return pixelMin.x == pixelMax.x || pixelMin.y == pixelMax.y;
}

bool meshPixelViewportCull(vec2 pixelMin, vec2 pixelMax)
{
  return pixelMax.x < 0.0 || pixelMin.x >= view.viewportf.x || pixelMax.y < 0.0 || pixelMin.y >= view.viewportf.y;
}

bool testTriangleHW(vec4 ha, vec4 hb, vec4 hc)
{
#if !USE_FORCED_TWO_SIDED
  if(!isFrontFacingHW(ha, hb, hc))
  {
    return false;
  }
#endif

  vec2 pixelMin = min(meshGetScreenPos(ha), min(meshGetScreenPos(hb), meshGetScreenPos(hc)));
  vec2 pixelMax = max(meshGetScreenPos(ha), max(meshGetScreenPos(hb), meshGetScreenPos(hc)));

  meshPixelBboxEpsilon(pixelMin, pixelMax);

  return !meshPixelViewportCull(pixelMin, pixelMax) && !meshPixelBboxCull(pixelMin, pixelMax);
}

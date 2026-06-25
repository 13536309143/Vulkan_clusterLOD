bool isFrontFacingHW(vec4 ha, vec4 hb, vec4 hc)
{
  return determinant(mat3(ha.xyw, hb.xyw, hc.xyw)) <= 0.0;
}

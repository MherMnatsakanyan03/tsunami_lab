

__constant float m_g = 9.80665;
__constant float m_gSqrt = 3.131557121;

inline void waveSpeeds(float i_hL, float i_hR, float i_uL, float i_uR,
                       float *o_waveSpeedL, float *o_waveSpeedR) {
  // Pre-compute square-root ops
  float l_hSqrtL = sqrt(i_hL);
  float l_hSqrtR = sqrt(i_hR);

  // Compute FWave averages
  float l_hRoe = 0.5f * (i_hL + i_hR);
  float l_uRoe = l_hSqrtL * i_uL + l_hSqrtR * i_uR;
  l_uRoe /= l_hSqrtL + l_hSqrtR;

  // Compute wave speeds
  float l_ghSqrtRoe = m_gSqrt * sqrt(l_hRoe);
  *o_waveSpeedL = l_uRoe - l_ghSqrtRoe;
  *o_waveSpeedR = l_uRoe + l_ghSqrtRoe;
}

inline void flux(float i_h, float i_hu, float *o_flux) {

  o_flux[0] = i_hu;
  o_flux[1] = i_hu * i_hu / i_h + m_g * (i_h * i_h * 0.5f);
}

inline void deltaXPsi(float i_hL, float i_hR, float i_bL, float i_bR,
                      float *o_deltaXPsi) {

  o_deltaXPsi[0] = 0;
  o_deltaXPsi[1] = -1 * m_g * (i_bR - i_bL) * (i_hL + i_hR) * 0.5f;
}

inline void waveStrengths(float i_hL, float i_hR, float i_huL, float i_huR,
                          float i_bL, float i_bR, float i_waveSpeedL,
                          float i_waveSpeedR, float *o_strengthL,
                          float *o_strengthR) {

  // Compute inverse of right eigenvector-matrix
  float l_detInv = 1 / (i_waveSpeedR - i_waveSpeedL);

  float l_rInv[2][2] = {0};
  l_rInv[0][0] = l_detInv * i_waveSpeedR;
  l_rInv[0][1] = -l_detInv;
  l_rInv[1][0] = -l_detInv * i_waveSpeedL;
  l_rInv[1][1] = l_detInv;

  float l_fluxL[2] = {0};
  float l_fluxR[2] = {0};

  // Die Funktion flux wird hier als Beispiel aufgerufen. Sie muss entsprechend
  // Ihrer Definition angepasst sein.
  flux(i_hL, i_huL, l_fluxL);
  flux(i_hR, i_huR, l_fluxR);

  float l_deltaXPsi[2] = {0};

  // Die Funktion deltaXPsi wird hier als Beispiel aufgerufen. Sie muss
  // entsprechend Ihrer Definition angepasst sein.
  deltaXPsi(i_hL, i_hR, i_bL, i_bR, l_deltaXPsi);

  float l_fluxJump[2];
  l_fluxJump[0] = l_fluxR[0] - l_fluxL[0] - l_deltaXPsi[0];
  l_fluxJump[1] = l_fluxR[1] - l_fluxL[1] - l_deltaXPsi[1];

  // Compute wave strengths
  *o_strengthL = l_rInv[0][0] * l_fluxJump[0];
  *o_strengthL += l_rInv[0][1] * l_fluxJump[1];
  *o_strengthR = l_rInv[1][0] * l_fluxJump[0];
  *o_strengthR += l_rInv[1][1] * l_fluxJump[1];
}

inline void netUpdates(float i_hL, float i_hR, float i_huL, float i_huR,
                       float i_bL, float i_bR, float *o_netUpdateL,
                       float *o_netUpdateR) {
  // both sides are dry -> exit 0
  if (i_hL <= 0 && i_hR <= 0) {
    o_netUpdateL[0] = 0;
    o_netUpdateL[1] = 0;
    o_netUpdateR[0] = 0;
    o_netUpdateR[1] = 0;
    return;
  }

  // if dry, then no need to update
  bool do_update_left = true;
  bool do_update_right = true;

  // left side dry -> reflect to right
  if (i_hL <= 0) {
    i_hL = i_hR;
    i_huL = -i_huR;
    i_bL = i_bR;
    do_update_left = false;
  }
  // right side dry -> reflect to left
  else if (i_hR <= 0) {
    i_hR = i_hL;
    i_huR = -i_huL;
    i_bR = i_bL;
    do_update_right = false;
  }

  // compute particle velocities
  float l_uL = i_huL / i_hL;
  float l_uR = i_huR / i_hR;

  // compute wave speeds
  float l_sL = 0;
  float l_sR = 0;
  waveSpeeds(i_hL, i_hR, l_uL, l_uR, &l_sL, &l_sR);

  // compute wave strengths
  float l_aL = 0;
  float l_aR = 0;
  waveStrengths(i_hL, i_hR, i_huL, i_huR, i_bL, i_bR, l_sL, l_sR, &l_aL, &l_aR);

  // compute waves
  float l_waveL[2] = {0};
  float l_waveR[2] = {0};

  l_waveL[0] = l_aL;
  l_waveL[1] = l_aL * l_sL;

  l_waveR[0] = l_aR;
  l_waveR[1] = l_aR * l_sR;

  // set net-updates depending on wave speeds
  for (int l_qt = 0; l_qt < 2; l_qt++) {
    // init
    o_netUpdateL[l_qt] = 0;
    o_netUpdateR[l_qt] = 0;

    // 1st wave
    if (l_sL < 0 && do_update_left) {
      o_netUpdateL[l_qt] += l_waveL[l_qt];
    } else if (l_sL >= 0 && do_update_right) {
      o_netUpdateR[l_qt] += l_waveL[l_qt];
    }

    // 2nd wave
    if (l_sR > 0 && do_update_right) {
      o_netUpdateR[l_qt] += l_waveR[l_qt];
    } else if (l_sR <= 0 && do_update_left) {
      o_netUpdateL[l_qt] += l_waveR[l_qt];
    }
  }
}

__kernel void updateXAxisKernel(__global float *i_hOld, __global float *i_huOld,
                                __global float *i_hvOld, __global float *i_b,
                                int m_nCells_x, int m_nCells_y, float i_scaling,
                                __global float *o_hNew, __global float *o_huNew,
                                __global float *o_hvNew) {

  int l_x = get_global_id(0);
  int l_y = get_global_id(1);

  int l_coord_L = l_y * (m_nCells_x + 2) + l_x;
  int l_coord_R = l_y * (m_nCells_x + 2) + l_x + 1;

  if (l_x > m_nCells_x || l_y > m_nCells_y)
    return;

  //   Define net updates array
  float l_netUpdatesL[2];
  float l_netUpdatesR[2];

  netUpdates(i_hOld[l_coord_L], i_hOld[l_coord_R], i_huOld[l_coord_L],
             i_huOld[l_coord_R], i_b[l_coord_L], i_b[l_coord_R], l_netUpdatesL,
             l_netUpdatesR);

  o_hNew[l_coord_L] -= i_scaling * l_netUpdatesL[0];
  o_huNew[l_coord_L] -= i_scaling * l_netUpdatesL[1];
  o_hNew[l_coord_R] -= i_scaling * l_netUpdatesR[0];
  o_huNew[l_coord_R] -= i_scaling * l_netUpdatesR[1];
}

__kernel void updateYAxisKernel(__global float *i_hOld, __global float *i_huOld,
                                __global float *i_hvOld, __global float *i_b,
                                int m_nCells_x, int m_nCells_y, float i_scaling,
                                __global float *o_hNew, __global float *o_huNew,
                                __global float *o_hvNew) {

  int l_x = get_global_id(0);
  int l_y = get_global_id(1);

  int l_coord_L = l_y * (m_nCells_x + 2) + l_x;
  int l_coord_R = (l_y + 1) * (m_nCells_x + 2) + l_x;

  if (l_x > m_nCells_x || l_y > m_nCells_y)
    return;

  //   Define net updates array
  float l_netUpdatesL[2];
  float l_netUpdatesR[2];

  netUpdates(i_hOld[l_coord_L], i_hOld[l_coord_R], i_hvOld[l_coord_L],
             i_hvOld[l_coord_R], i_b[l_coord_L], i_b[l_coord_R], l_netUpdatesL,
             l_netUpdatesR);

  o_hNew[l_coord_L] -= i_scaling * l_netUpdatesL[0];
  o_hvNew[l_coord_L] -= i_scaling * l_netUpdatesL[1];
  o_hNew[l_coord_R] -= i_scaling * l_netUpdatesR[0];
  o_hvNew[l_coord_R] -= i_scaling * l_netUpdatesR[1];
}

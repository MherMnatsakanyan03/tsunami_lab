__constant float m_g = 9.80665;
__constant float m_gSqrt = 3.131557121;

inline void waveSpeeds(float i_hL, float i_hR, float i_uL, float i_uR,
                       float *o_waveSpeedL, float *o_waveSpeedR) {
  // Pre-compute square-root ops
  float l_hSqrtL = native_sqrt(i_hL);
  float l_hSqrtR = native_sqrt(i_hR);

  // Compute FWave averages
  float l_hRoe = 0.5f * (i_hL + i_hR);
  float l_uRoe = l_hSqrtL * i_uL + l_hSqrtR * i_uR;
  l_uRoe /= l_hSqrtL + l_hSqrtR;

  // Compute wave speeds
  float l_ghSqrtRoe = m_gSqrt * native_sqrt(l_hRoe);
  *o_waveSpeedL = l_uRoe - l_ghSqrtRoe;
  *o_waveSpeedR = l_uRoe + l_ghSqrtRoe;
}

inline void flux(float i_h, float i_hu, float *o_flux) {
  int id = get_global_id(0);

  o_flux[id * 2] = i_hu;
  o_flux[id * 2 + 1] = i_hu * i_hu / i_h + m_g * (0.5f * i_h * i_h);
}

inline void deltaXPsi(float i_hL, float i_hR, float i_bL, float i_bR,
                      float *o_deltaXPsi) {
  int id = get_global_id(0);

  o_deltaXPsi[id * 2] = 0;
  o_deltaXPsi[id * 2 + 1] = -1 * m_g * (i_bR - i_bL) * (i_hL + i_hR) * 0.5f;
}

inline void waveStrengths(float i_hL, float i_hR, float i_huL, float i_huR,
                          float i_bL, float i_bR, float i_waveSpeedL,
                          float i_waveSpeedR, float *o_strengthL,
                          float *o_strengthR) {
  int id = get_global_id(0);

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
  o_strengthL[id] = l_rInv[0][0] * l_fluxJump[0] + l_rInv[0][1] * l_fluxJump[1];
  o_strengthR[id] = l_rInv[1][0] * l_fluxJump[0] + l_rInv[1][1] * l_fluxJump[1];
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

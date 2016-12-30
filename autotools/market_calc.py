#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" Market Calculation

仿真市场成交
依据数据拟合 损益，成单率，成手率，使其满足
期望损益 － 损益 / 损益 <= 0.01
期望成单率 － 成单率 / 成单率 <= 0.01
期望成手率 － 成手率 / 成手率 <= 0.01
"""

import autotools
import cffi
import os

def market_calc(cfg):
    pass

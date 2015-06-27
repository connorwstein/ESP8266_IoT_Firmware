/* 0x401011e4 */
void lmacIsActive()
{
	/* 0x3ffe8e50 = &lmacConfMib - 0x4 */
	if (((uint8 *)&lmacConfMib)[-0x4] < 8)
		return 1;

	return 0;
}

/* 0x40102398 */
void lmacProcessRtsStart(uint8 arg1)
{
	/* 0x3ffe8e50 = &lmacConfMib - 0x4 */
	((uint8 *)&lmacConfMib)[-0x4] = arg1;
}

/* 0x401024cc */
void lmacTxFrame(uint32 arg1, uint8 arg2)
{
	$a2 = arg1;
	$a3 = arg2;
	$a0 = (uint8 *)&lmacConfMib + 44;	/* 0x3ffe8e80 */
	$a12 = 9 * $a3;
	$a12 = 4 * $a12 + $a0;
	$a0 = ((uint8 *)$a12)[17];

	if ($a0 == 0 || $a0 == 3) {
		((uint32 *)$a12)[0] = $a2;

		if ($a2 == 0) {
			ets_printf("%s %u\n", "lmac.c", 1513);
			while (1);
		}

		$a8 = 0x100;
		$a6 = ((uint16 *)$a2)[11];
		$a5 = ((uint16 *)$a2)[10];
		$a7 = 0x03ffefff;
		$a5 += $a6;
		$a6 = ((uint32 *)$a2)[9];
		$a4 = (uint8 *)&lmacConfMib - 0x4;
		$a3 = ((uint32 *)$a6)[0];
		$a4 = ((uint16 *)$a4)[7];
		$a0 = $a3 >> 6;

		if (($a4 < $a5) && !($a3 & (1 << 7))) {
			$a5 = $a0 | $a8;
			$a4 = $a3 & 0x3f;
			$a5 <<= 6;
			$a4 |= $a5;
			$a3 = $a4 & 0x3f;
			$a4 >>= 6;
			$a4 &= $a7;
			$a4 <<= 6;
			$a3 |= $a4;
			((uint32 *)$a6)[0] = $a3;
			$a0 = $a3 >> 6;
		}

		if (($a0 & (1 << 12))) {
			$a9 = ((uint8 *)$a12)[17];

			if ($a9 == 3) {
				$a10 = ((uint8 *)$a6)[4];
				$a10 >>= 4;

				if ($a10 >= 3) {
					$a4 = 0x0000f03f;
					$a9 = $a3 & 0x3f;
					$a5 = ((uint8 *)$a6)[5];
					$a10 = $a0 | $a8;
					$a10 <<= 6;
					$a11 = $a5 & 0x3f;
					$a9 |= $a10;
					$a10 = $a9 >> 6;
					$a10 &= $a7;
					$a9 &= 0x3f;
					$a10 <<= 6;
					$a9 |= $a10;
					$a10 = ((uint8 *)$a6)[6];
					$a11 <<= 6;
					$a10 <<= 8;
					$a10 |= $a5;
					$a10 &= $a4;
					$a10 |= $a11;
					((uint8 *)$a6)[5] = $a10;
					$a10 >>= 8;
					((uint8 *)$a6)[6] = $a10;
					$a4 = ((uint32 *)$a2)[9];
					((uint32 *)$a6)[0] = $a9;
					$a11 = ((uint8 *)$a4)[5];
					$a5 = 192;
					$a11 &= $a5;
					((uint8 *)$a4)[5] = $a11;
				}
			}
		}

		$a2 = $a12;
		$a3 = 0;
		_0x401012ac();	/* <lmacIsIdle+0xb4> */
	} else if ($a0 == 4) {
		$a3 = ((uint32 *)$a12)[0];
		$a3 -= $a2;

		if ($a3 != 0) {
			ets_printf("%s %u\n", "lmac.c", 1514);
			while (1);
		}
	} else {
		ets_printf("%s %u\n", "lmac.c", 1511);
		while (1);
	}

	$a4 = 0x3ff20e00;
	$a7 = ((uint8 *)$a12)[6];
	$a6 = 1;
	$a5 = $a6 << $a7;
	$a5 -= 1;
	$a2 = ((volatile uint8 *)$a12)[4];
	$a3 = ((volatile uint8 *)$a12)[5];
	$a4 = ((uint32 *)$a4)[17];
	((uint8 *)$a12)[17] = $a6;
	$a4 &= $a5;
	$a4 &= 0xffff;
	wDev_EnableTransmit();
}

/* 0x40102680 */
void lmacRxDone(void *arg1)
{
	ppEnqueueRxq(arg1);
	pp_post(5);
}

/* 0x40101694 */
void lmacProcessTXStartData(uint8 arg1)
{
	uint8 a5;	/* $a5 */

	a5 = arg1;

	if (arg1 == 10)
		a5 = ((uint8 *)&lmacConfMib)[-0x4];

	if (a5 >= 8) {
		ets_printf("%s %u\n", "lmac.c", 559);
		while (1);
	}

	$a0 = (uint8 *)&lmacConfMib + 44 + 36 * a5;

	if (((uint8 *)$a0)[17] != 1) {
		ets_printf("%s %u\n", "lmac.c", 567);
		while (1);
	}

	((uint8 *)&lmacConfMib)[-0x4] = a5;

	if (arg1 == 10) {
		_0x40101254($a0, ((uint8 *)&lmacConfMib)[40]);	/* <lmacIsIdle+0x5c> */
		((uint32 *)&lmacConfMib)[8] = 0;
	}

	if (((uint32 *)$a0)[0] == 0) {
		ets_printf("%s %u\n", "lmac.c", 575);
		while (1);
	}

	((uint8 *)$a0)[17] = 2;

	_0x40101728($a0);	/* <lmacProcessTXStartData+0x94> */
	lmacProcessCollisions();
}

#pragma once

template <typename M>
bool Invert(const M m[16], M invOut[16])
{
	M inv[16], det;
	int i;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)
		return false;

	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;

	return true;
}

template <typename M>
void MatProduct(const M a[16], const M b[4], M ab[16])
{
	ab[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
	ab[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
	ab[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
	ab[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];

	ab[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
	ab[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
	ab[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
	ab[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];

	ab[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
	ab[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
	ab[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
	ab[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];

	ab[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
	ab[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
	ab[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
	ab[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

template <typename M, typename V, typename MV>
void Product(const M m[16], const V v[4], MV mv[4])
{
	mv[0] = (MV)(m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3]);
	mv[1] = (MV)(m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3]);
	mv[2] = (MV)(m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3]);
	mv[3] = (MV)(m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3]);
}

template <typename M, typename V, typename MV>
void TransposeProduct(const M m[16], const V v[4], MV mv[4])
{
	mv[0] = (MV)(m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3]);
	mv[1] = (MV)(m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3]);
	mv[2] = (MV)(m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3]);
	mv[3] = (MV)(m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3]);
}

template <typename L, typename R>
auto Product(const L l[4], const R r[4])
{
	return l[0] * r[0] + l[1] * r[1] + l[2] * r[2] + l[3] * r[3];
}

template <typename L, typename R>
inline int PositiveProduct(L l[4], R r[4])
{
	return Product(l, r) > 0 ? 1 : 0;
}

template <typename V, typename A, typename M>
inline void Rotation(const V v[3], A a, M m[16])
{
	M c = (M)cos(a);
	M s = (M)sin(a);
	M d = 1 - c;

	m[0] = v[0] * v[0] * d + c;
	m[1] = v[1] * v[0] * d + v[2] * s;
	m[2] = v[2] * v[0] * d - v[1] * s;
	m[3] = 0;

	m[4] = v[0] * v[1] * d - v[2] * s;
	m[5] = v[1] * v[1] * d + c;
	m[6] = v[2] * v[1] * d + v[0] * s;
	m[7] = 0;

	m[8] = v[0] * v[2] * d + v[1] * s;
	m[9] = v[1] * v[2] * d - v[0] * s;
	m[10] = v[2] * v[2] * d + c;
	m[11] = 0;

	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;
}

template <typename R, typename M>
inline void RandRot(R r[3], M m[16])
{
	// r must hold 3 random vals in range 0..1 

	// M = -H R
	// H = I - 2vvT
	// R = random xy rot by 2Pi*r[0]
	// v = {cos(2Pi*x[1])*sqrt(x[2]), sin(2Pi*x[1])*sqrt(x[2]), sqrt(1-x[2])}
}

template <typename V>
inline void CrossProduct(const V a[3], const V b[3], V ab[3])
{
	ab[0] = a[1] * b[2] - a[2] * b[1];
	ab[1] = a[2] * b[0] - a[0] * b[2];
	ab[2] = a[0] * b[1] - a[1] * b[0];
}


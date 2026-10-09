#define TT_CBUFFER(n, r) cbuffer n : register(r)
#define TT_CBUFFER_END
#define REG(r) : register(r)
#define SEM(s) : s
#define NUMTHREADS(x, y, z) [numthreads(x, y, z)]
#define GROUPSHARED groupshared
#define NOINTERP nointerpolation
#define BARRIER GroupMemoryBarrierWithGroupSync()
#define OUT(T) out T

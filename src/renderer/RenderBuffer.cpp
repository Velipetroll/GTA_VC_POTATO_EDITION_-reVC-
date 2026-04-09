#include "common.h"

#include "RenderBuffer.h"

int32 TempBufferVerticesStored;
int32 TempBufferIndicesStored;

VertexBufferUnion TempVertexBuffer;
RwImVertexIndex TempBufferRenderIndexList[TEMPBUFFERINDEXSIZE];

int RenderBuffer::VerticesToBeStored;
int RenderBuffer::IndicesToBeStored;

void
RenderBuffer::ClearRenderBuffer(void)
{
	TempBufferVerticesStored = 0;
	TempBufferIndicesStored = 0;
}

void
RenderBuffer::StartStoring(int numIndices, int numVertices, RwImVertexIndex **indexStart, RwIm3DVertex **vertexStart)
{
	// Optimización: Unificación de evaluación de rama (Branching)
	if (TempBufferIndicesStored + numIndices >= TEMPBUFFERINDEXSIZE || TempBufferVerticesStored + numVertices >= TEMPBUFFERVERTSIZE)
		RenderStuffInBuffer();

	*indexStart = &TempBufferRenderIndexList[TempBufferIndicesStored];
	*vertexStart = &TempBufferRenderVertices[TempBufferVerticesStored];
	IndicesToBeStored = numIndices;
	VerticesToBeStored = numVertices;
}

void
RenderBuffer::StopStoring(void)
{
	// Optimización extrema para CPUs limitadas: Aritmética de punteros en lugar de indexación de arrays.
	// Evita multiplicaciones de índice por tamaño de variable en cada ciclo del bucle.
	RwImVertexIndex* pIndex = &TempBufferRenderIndexList[TempBufferIndicesStored];
	RwImVertexIndex* pEnd = pIndex + IndicesToBeStored;
	int32 offset = TempBufferVerticesStored;

	while (pIndex < pEnd) {
		*pIndex += offset;
		pIndex++;
	}

	TempBufferIndicesStored += IndicesToBeStored;
	TempBufferVerticesStored += VerticesToBeStored;
}

void
RenderBuffer::RenderStuffInBuffer(void)
{
	if (TempBufferVerticesStored && RwIm3DTransform(TempBufferRenderVertices, TempBufferVerticesStored, nil, rwIM3D_VERTEXUV)) {
		RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TempBufferRenderIndexList, TempBufferIndicesStored);
		RwIm3DEnd();
	}

	// Optimización: Inlining manual para evitar el salto (Call/Ret) a ClearRenderBuffer()
	TempBufferVerticesStored = 0;
	TempBufferIndicesStored = 0;
}
// plane mesh
// Quad mesh made of many quads. Default is 100x100
#include "planemesh.h"

// Initialise buffer and load texture.
PlaneMesh::PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int lresolution)
{
	resolution = lresolution;
	initBuffers(device);
}

// Release resources.
PlaneMesh::~PlaneMesh()
{
	// Run parent deconstructor
	BaseMesh::~BaseMesh();
}

// Generate plane (including texture coordinates and normals).
void PlaneMesh::initBuffers(ID3D11Device* device) {
	int resX = resolution;
	int resY = resolution;

	float incrX = 1.f / resX;
	float incrY = 1.f / resY;

	int index = 0;
	float u = 0;
	float v = 0;
	
	// Calculate the number of vertices in the terrain mesh.
	vertexCount = resX * resY * 6;
	indexCount = vertexCount;

	VertexType *vertices = new VertexType[vertexCount];
	unsigned long *indices = new unsigned long[indexCount];
	
	for (int j = 0; j < resY; ++j) {
		for (int i = 0; i < resX; ++i) {
			// -- Front face -----------------------------------------------------------------
			
			// Upper left.
			vertices[index + 0].position = XMFLOAT3(i, 0.0f, j);
			vertices[index + 0].texture  = XMFLOAT2(u, v);
			vertices[index + 0].normal   = XMFLOAT3(0.0, 1.0, 0.0);

			// Upper right.
			vertices[index + 1].position = XMFLOAT3(i + 1, 0.0f, j + 1);
			vertices[index + 1].texture = XMFLOAT2(u + incrX, v + incrY);
			vertices[index + 1].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// lower left
			vertices[index + 2].position = XMFLOAT3(i, 0.0f, j + 1);
			vertices[index + 2].texture = XMFLOAT2(u, v + incrY);
			vertices[index + 2].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Upper left
			vertices[index + 3].position = XMFLOAT3(i, 0.0f, j);
			vertices[index + 3].texture = XMFLOAT2(u, v);
			vertices[index + 3].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Bottom right
			vertices[index + 4].position = XMFLOAT3(i + 1, 0.0f, j);
			vertices[index + 4].texture = XMFLOAT2(u + incrX, v);
			vertices[index + 4].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Upper right.
			vertices[index + 5].position = XMFLOAT3(i + 1, 0.0f, j + 1);
			vertices[index + 5].texture = XMFLOAT2(u + incrX, v + incrY);
			vertices[index + 5].normal = XMFLOAT3(0.0, 1.0, 0.0);

			indices[index + 0] = index + 0;
			indices[index + 1] = index + 1;
			indices[index + 2] = index + 2;
			indices[index + 3] = index + 3;
			indices[index + 4] = index + 4;
			indices[index + 5] = index + 5;

			u += incrX;
			index += 6;
		}

		u = 0;
		v += incrY;
	}

	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType)* vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData{};
	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	// Now create the vertex buffer.
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);
	
	D3D11_BUFFER_DESC indexBufferDesc{};
	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long)* indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData{};
	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	// Create the index buffer.
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
	
	// Release the arrays now that the buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;
}



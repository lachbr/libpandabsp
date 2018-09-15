//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
/**
 * \file MeshLoaderObj.cpp
 *
 * \date 2016-03-16
 * \author Mikko Mononen (modified by consultit)
 */

#include "MeshLoaderObj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <nodePathCollection.h>
#include <geomVertexReader.h>

namespace rnsup
{

rcMeshLoaderObj::rcMeshLoaderObj() :
	m_scale(1.0f),
	m_verts(0),
	m_tris(0),
	m_normals(0),
	m_vertCount(0),
	m_triCount(0),
	m_currentMaxIndex(0),
	vcap(0),
	tcap(0)
{
	for (int i = 0; i < 3; ++i)
	{
		m_translation[i] = 0.0;
	}
}

rcMeshLoaderObj::~rcMeshLoaderObj()
{
	delete [] m_verts;
	m_verts = NULL;
	delete [] m_normals;
	m_normals = NULL;
	delete [] m_tris;
	m_tris = NULL;
}
		
void rcMeshLoaderObj::addVertex(float x, float y, float z, int& cap)
{
	if (m_vertCount+1 > cap)
	{
		cap = !cap ? 8 : cap*2;
		float* nv = new float[cap*3];
		if (m_vertCount)
			memcpy(nv, m_verts, m_vertCount*3*sizeof(float));
		delete [] m_verts;
		m_verts = nv;
	}
	float* dst = &m_verts[m_vertCount*3];
	*dst++ = x*m_scale + m_translation[0];
	*dst++ = y*m_scale + m_translation[1];
	*dst++ = z*m_scale + m_translation[2];
	m_vertCount++;
}

void rcMeshLoaderObj::addTriangle(int a, int b, int c, int& cap)
{
	if (m_triCount+1 > cap)
	{
		cap = !cap ? 8 : cap*2;
		int* nv = new int[cap*3];
		if (m_triCount)
			memcpy(nv, m_tris, m_triCount*3*sizeof(int));
		delete [] m_tris;
		m_tris = nv;
	}
	int* dst = &m_tris[m_triCount*3];
	*dst++ = a;
	*dst++ = b;
	*dst++ = c;
	m_triCount++;
}
} //rnsup

static char* parseRow(char* buf, char* bufEnd, char* row, int len)
{
	bool start = true;
	bool done = false;
	int n = 0;
	while (!done && buf < bufEnd)
	{
		char c = *buf;
		buf++;
		// multirow
		switch (c)
		{
			case '\\':
				break;
			case '\n':
				if (start) break;
				done = true;
				break;
			case '\r':
				break;
			case '\t':
			case ' ':
				if (start) break;
			default:
				start = false;
				row[n++] = c;
				if (n >= len-1)
					done = true;
				break;
		}
	}
	row[n] = '\0';
	return buf;
}

static int parseFace(char* row, int* data, int n, int vcnt)
{
	int j = 0;
	while (*row != '\0')
	{
		// Skip initial white space
		while (*row != '\0' && (*row == ' ' || *row == '\t'))
			row++;
		char* s = row;
		// Find vertex delimiter and terminated the string there for conversion.
		while (*row != '\0' && *row != ' ' && *row != '\t')
		{
			if (*row == '/') *row = '\0';
			row++;
		}
		if (*s == '\0')
			continue;
		int vi = atoi(s);
		data[j++] = vi < 0 ? vi+vcnt : vi-1;
		if (j >= n) return j;
	}
	return j;
}

namespace rnsup
{ 
bool rcMeshLoaderObj::load(const std::string& filename, float scale, float* translation)
{
	char* buf = 0;
	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp)
		return false;
	m_scale = scale;
	if (translation != NULL)
	{
		m_translation[0] = translation[0];
		m_translation[1] = translation[1];
		m_translation[2] = translation[2];
	}
	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return false;
	}
	long bufSize = ftell(fp);
	if (bufSize < 0)
	{
		fclose(fp);
		return false;
	}
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return false;
	}
	buf = new char[bufSize];
	if (!buf)
	{
		fclose(fp);
		return false;
	}
	size_t readLen = fread(buf, bufSize, 1, fp);
	fclose(fp);

	if (readLen != 1)
	{
		delete[] buf;
		return false;
	}

	char* src = buf;
	char* srcEnd = buf + bufSize;
	char row[512];
	int face[32];
	float x,y,z;
	int nv;
	int vcap = 0;
	int tcap = 0;
	
	while (src < srcEnd)
	{
		// Parse one row
		row[0] = '\0';
		src = parseRow(src, srcEnd, row, sizeof(row)/sizeof(char));
		// Skip comments
		if (row[0] == '#') continue;
		if (row[0] == 'v' && row[1] != 'n' && row[1] != 't')
		{
			// Vertex pos
			sscanf(row+1, "%f %f %f", &x, &y, &z);
			addVertex(x, y, z, vcap);
		}
		if (row[0] == 'f')
		{
			// Faces
			nv = parseFace(row+1, face, 32, m_vertCount);
			for (int i = 2; i < nv; ++i)
			{
				const int a = face[0];
				const int b = face[i-1];
				const int c = face[i];
				if (a < 0 || a >= m_vertCount || b < 0 || b >= m_vertCount || c < 0 || c >= m_vertCount)
					continue;
				addTriangle(a, b, c, tcap);
			}
		}
	}

	delete [] buf;

	// Calculate normals.
	m_normals = new float[m_triCount*3];
	for (int i = 0; i < m_triCount*3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i]*3];
		const float* v1 = &m_verts[m_tris[i+1]*3];
		const float* v2 = &m_verts[m_tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1]*e1[2] - e0[2]*e1[1];
		n[1] = e0[2]*e1[0] - e0[0]*e1[2];
		n[2] = e0[0]*e1[1] - e0[1]*e1[0];
		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
	
	m_filename = filename;
	return true;
}

bool rcMeshLoaderObj::load(NodePath model, NodePath referenceNP)
{
	//reset scale & translation
	m_scale = 1.0;
	m_translation[0] = 0.0;
	m_translation[1] = 0.0;
	m_translation[2] = 0.0;
	//reset max index
	m_currentMaxIndex = 0;
	//all transform are applied wrt reference node
	//get current model transform
	m_currentTranformMat = model.get_transform(referenceNP)->get_mat();
	///Elaborate
	//Walk through all the model's GeomNodes
	NodePathCollection geomNodeCollection;
	// check if model is GeomNode itself (i.e. when procedurally generated)
	if (model.node()->is_of_type(GeomNode::get_class_type()))
	{
		geomNodeCollection.add_path(model);
	}
	else
	{
		//get all GeomNodes for the hierarchy below model
		geomNodeCollection = model.find_all_matches("**/+GeomNode");
	}
	//
	int numPaths = geomNodeCollection.get_num_paths();
	PRINT_DEBUG("\tGeomNodes number: " << numPaths);
	for (int i = 0; i < numPaths; i++)
	{
		PT(GeomNode)geomNode = DCAST(GeomNode,geomNodeCollection.get_path(i).node());
		processGeomNode(geomNode);
	}

	// Calculate normals.
	m_normals = new float[m_triCount * 3];
	for (int i = 0; i < m_triCount * 3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i] * 3];
		const float* v1 = &m_verts[m_tris[i + 1] * 3];
		const float* v2 = &m_verts[m_tris[i + 2] * 3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1] * e1[2] - e0[2] * e1[1];
		n[1] = e0[2] * e1[0] - e0[0] * e1[2];
		n[2] = e0[0] * e1[1] - e0[1] * e1[0];
		float d = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (d > 0)
		{
			d = 1.0f / d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}

	return true;
}

void rcMeshLoaderObj::processGeomNode(PT(GeomNode)geomNode)
{
	//Walk through the list of GeomNode's Geoms
	int numGeoms = geomNode->get_num_geoms();
	PRINT_DEBUG("\t Geoms number: " << numGeoms);
	for (int j = 0; j < numGeoms; j++)
	{
		CPT(Geom)geom = geomNode->get_geom(j);
		processGeom(geom);
	}
}

void rcMeshLoaderObj::processGeom(CPT(Geom)geom)
{
	//check if there are triangles
	if (geom->get_primitive_type() != GeomEnums::PT_polygons)
	{
		return;
	}
	//insert geom
	m_geoms.push_back(geom);
	unsigned int geomIndex = m_geoms.size() - 1;
	CPT(GeomVertexData)vertexData = geom->get_vertex_data();
	//Process vertices
	processVertexData(vertexData);
	int numPrimitives = geom->get_num_primitives();
	PRINT_DEBUG("\t  Primitives number: " << numPrimitives);
	//Walk through the list of Geom's GeomPrimitives
	for(int i=0;i<numPrimitives;i++)
	{
		CPT(GeomPrimitive) primitive = geom->get_primitive(i);
		processPrimitive(primitive, geomIndex);
	}
}

void rcMeshLoaderObj::processVertexData(CPT(GeomVertexData)vertexData)
{
	//check if vertexData already present
	unsigned int vertexDataIndex = m_vertexData.size();
	unsigned int index = 0;
	while(index < vertexDataIndex)
	{
		if(m_vertexData[index] == vertexData)
		{
			break;
		}
		++index;
	}
	if(index == vertexDataIndex)
	{
		//vertexData is not present
		GeomVertexReader vertexReader = GeomVertexReader(vertexData, "vertex");
		while (!vertexReader.is_at_end())
		{
			LVector3f vertex = vertexReader.get_data3f();
			m_currentTranformMat.xform_point_in_place(vertex);
			//add vertex
			float pvertex[3];
			LVecBase3fToRecast(vertex, pvertex);
			addVertex(pvertex[0], pvertex[1], pvertex[2], vcap);
		}
		m_startIndices.push_back(m_currentMaxIndex);
		//increment the max index
		int vertexNum = vertexData->get_num_rows();
		m_currentMaxIndex += vertexNum;
	}
	else
	{
		m_startIndices.push_back(m_startIndices[index]);
	}
	//insert vertexData any way
	m_vertexData.push_back(vertexData);
	PRINT_DEBUG("\t   Vertices number: " << vertexData->get_num_rows() <<
	" - Start index: " << m_startIndices.back());
}

void rcMeshLoaderObj::processPrimitive(CPT(GeomPrimitive)primitive, unsigned int geomIndex)
{
	PRINT_DEBUG("\t---");
	PRINT_DEBUG("\t   Primitive type: " <<
			primitive->get_type().get_name() <<
			" - number: " << primitive->get_num_primitives());
	//decompose to triangles
	CPT(GeomPrimitive)primitiveDec = primitive->decompose();
	int numPrimitives = primitiveDec->get_num_primitives();
	PRINT_DEBUG("\t   Decomposed Primitive type: " <<
			primitiveDec->get_type().get_name() <<
			" - number: " << numPrimitives);
	for (int k = 0; k < numPrimitives; k++)
	{
		int s = primitiveDec->get_primitive_start(k);
		int e = primitiveDec->get_primitive_end(k);
		//add vertex indices
		int vi[3];
		for (int j = s; j < e; ++j)
		{
			vi[j-s] = primitiveDec->get_vertex(j) +
					m_startIndices[geomIndex];
		}
		addTriangle(vi[0], vi[1], vi[2], tcap);
	}
}

rcMeshLoaderObj& rcMeshLoaderObj::operator=(const rcMeshLoaderObj& copy)
{
	m_filename = copy.m_filename;
	m_scale = copy.m_scale;
	m_translation[0] = copy.m_translation[0];
	m_translation[1] = copy.m_translation[1];
	m_translation[2] = copy.m_translation[2];
	//vertices
	m_vertCount = copy.m_vertCount;
	m_verts = new float[m_vertCount * 3];
	for (int v = 0; v < m_vertCount * 3; ++v)
	{
		m_verts[v] = copy.m_verts[v];
	}
	//triangles
	m_triCount = copy.m_triCount;
	m_tris = new int[m_triCount * 3];
	for (int t = 0; t < m_triCount * 3; ++t)
	{
		m_tris[t] = copy.m_tris[t];
	}
	//normals
	m_normals = new float[m_triCount * 3];
	for (int n = 0; n < m_triCount * 3; ++n)
	{
		m_normals[n] = copy.m_normals[n];
	}
	//Model stuff
	m_geoms = copy.m_geoms;
	m_vertexData = copy.m_vertexData;
	m_startIndices = copy.m_startIndices;
	m_currentTranformMat = copy.m_currentTranformMat;
	m_currentMaxIndex = copy.m_currentMaxIndex;
	vcap = copy.vcap;
	tcap = copy.tcap;
	//
	return *this;
}

void rcMeshLoaderObj::write_datagram(Datagram &dg) const
{
	dg.add_string(m_filename);
	dg.add_stdfloat(m_scale);
	dg.add_stdfloat(m_translation[0]);
	dg.add_stdfloat(m_translation[1]);
	dg.add_stdfloat(m_translation[2]);
	//vertices
	dg.add_int32(m_vertCount);
	for (int v = 0; v < m_vertCount * 3; ++v)
	{
		dg.add_stdfloat(m_verts[v]);
	}
	//triangles
	dg.add_int32(m_triCount);
	for (int t = 0; t < m_triCount * 3; ++t)
	{
		dg.add_int32(m_tris[t]);
	}
	//normals
	for (int n = 0; n < m_triCount * 3; ++n)
	{
		dg.add_stdfloat(m_normals[n]);
	}

	///Model stuff not written: not needed for rebuilding
}

void rcMeshLoaderObj::read_datagram(DatagramIterator &scan)
{
	m_filename = scan.get_string();
	m_scale = scan.get_stdfloat();
	m_translation[0] = scan.get_stdfloat();
	m_translation[1] = scan.get_stdfloat();
	m_translation[2] = scan.get_stdfloat();
	//vertices
	m_vertCount = scan.get_int32();
	m_verts = new float[m_vertCount * 3];
	for (int v = 0; v < m_vertCount * 3; ++v)
	{
		m_verts[v] = scan.get_stdfloat();
	}
	//triangles
	m_triCount = scan.get_int32();
	m_tris = new int[m_triCount * 3];
	for (int t = 0; t < m_triCount * 3; ++t)
	{
		m_tris[t] = scan.get_int32();
	}
	//normals
	m_normals = new float[m_triCount * 3];
	for (int n = 0; n < m_triCount * 3; ++n)
	{
		m_normals[n] = scan.get_stdfloat();
	}

	///Model stuff not read: not needed for rebuilding
}

} // namespace rnsup

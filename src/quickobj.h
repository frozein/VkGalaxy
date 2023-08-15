/* ------------------------------------------------------------------------
 * 
 * quickobj.h
 * author: Daniel Elwell (2023)
 * license: MIT
 * description: a single-header library for loading 3D meshes from .obj files, and their
 * associated materials from .mtl files
 * 
 * ------------------------------------------------------------------------
 * 
 * NOTE: only supports .obj files with triangles and quads, n-gons will result in an error
 * 
 * to use you must "#define QOBJ_IMPLEMENTATION" in exactly one source file before including the library
 * 
 * the following strutures and functions are defined for end use:
 * (all other functions/structures are meant for internal library use only and do not have documentation)
 * 
 * STRUCTURES:
 * ------------------------------------------------------------------------
 * QOBJvec2, QOBJvec3:
 * 		vectors of floats
 * QOBJvertex 
 * 		a single triangle vertex
 * 		layout: position (vec3); normal (vec3); texture coordinate (vec2)
 * QOBJmaterial
 * 		a single material (non-PBR)
 * 		contains:
 * 			ambient color (vec3); diffuse color (vec3); specular color (vec3)
 * 			ambient color map (char*); diffuse color map (char*); specular color map (char*); normal map (char*)
 * 			opacity (float); shininess/specular exponent (float); refraction index (float)
 * 		NOTE: if a map is NULL, then it does not exist, otherwise, it contains the path to the texture
 * QOBJmesh
 * 		a mesh with a single material, uses an index buffer
 * 		contains:
 * 			number of vertices (size_t); vertex buffer capacity (size_t) (for internal use, please ignore)
 * 			array of vertices (QOBJvertex*)
 * 			number of indices (size_t); index buffer capacity (size_t) (for internal use, please ignore)
 * 			array of indices (uint32_t*)
 * 			material index (uint32_t)
 * 		NOTE: the mesh MUST be rendered with the index buffer
 * 
 * FUNCTIONS:
 * ------------------------------------------------------------------------
 * qobj_load(const char* path, size_t* numMeshes, QOBJmesh** meshes, size_t* numMaterials, QOBJmaterial** materials)
 * 		loads a .obj file from [path], including any necessary .mtl files
 * 		the [numMeshes] and [numMaterials] fields are populated with the number of meshes and materials loaded. respectively
 * 		the [meshes] and [materials] fields are populated with the array of meshes and materials loaded, respectively
 * 		NOTE: in order to render the entire model, you must render each mesh in the array, using its corresponding material (the material index field)
 * qobj_free(size_t numMeshes, QOBJmesh* meshes, size_t numMaterials, QOBJmaterial* materials)
 * 		frees the memory created by a call to qobj_load, must be called in order to prevent memory leaks
 */

#ifndef QOBJ_H
#define QOBJ_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>

//----------------------------------------------------------------------//
//DECLARATIONS:

//a 2-dimensional vector of floats
typedef struct QOBJvec2
{
	float v[2];
} QOBJvec2;

//a 3-dimensional vector of floats
typedef struct QOBJvec3
{
	float v[3];
} QOBJvec3;

//a single vertex with position, normal, and texture coordinates
typedef struct QOBJvertex
{
	QOBJvec3 pos;
	QOBJvec3 normal;
	QOBJvec2 texCoord;
} QOBJvertex;

//a material (not pbr)
typedef struct QOBJmaterial
{
	char* name;

	QOBJvec3 ambientColor;
	QOBJvec3 diffuseColor;
	QOBJvec3 specularColor;
	char* ambientMapPath;  //== NULL if one does not exist
	char* diffuseMapPath;  //== NULL if one does not exist
	char* specularMapPath; //== NULL if one does not exist
	char* normalMapPath;   //== NULL if one does not exist

	float opacity;
	float specularExp;
	float refractionIndex;
} QOBJmaterial;

//a mesh consisting of vertices and indices
typedef struct QOBJmesh
{
	size_t numVertices;
	size_t vertexCap;
	QOBJvertex* vertices;

	size_t numIndices;
	size_t indexCap;
	uint32_t* indices;

	uint32_t materialIdx;
} QOBJmesh;

//an error value, returned by all functions which can have errors
typedef enum QOBJerror
{
	QOBJ_SUCCESS = 0,
	QOBJ_ERROR_INVALID_FILE,
	QOBJ_ERROR_IO,
	QOBJ_ERROR_OUT_OF_MEM,
	QOBJ_ERROR_UNSUPPORTED_DATA_TYPE
} QOBJerror;

QOBJerror qobj_load(const char* path, size_t* numMeshes, QOBJmesh** meshes, size_t* numMaterials, QOBJmaterial** materials);
void qobj_free(size_t numMeshes, QOBJmesh* meshes, size_t numMaterials, QOBJmaterial* materials);

//----------------------------------------------------------------------//

#ifdef QOBJ_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <malloc.h>

//----------------------------------------------------------------------//
//IMPLEMENTATION STRUCTS:

//a 3-dimensional vector of unsigned integers
typedef struct QOBJuvec3
{
	uint32_t v[3];
} QOBJuvec3;

//a hashmap with a vec3 of vertex data indices for keys
typedef struct QOBJvertexHashmap
{
	size_t size;
	size_t cap;
	QOBJuvec3* keys; //an x component of UINT32_MAX signifies an unused index
	uint32_t* vals;
} QOBJvertexHashmap;

//----------------------------------------------------------------------//
//HASH MAP FUNCTIONS:

QOBJerror qobj_hashmap_create(QOBJvertexHashmap* map)
{
	map->size = 0;
	map->cap = 32;
	map->keys = (QOBJuvec3*)malloc(map->cap * sizeof(QOBJuvec3));
	if(!map->keys)
		return QOBJ_ERROR_OUT_OF_MEM;
	map->vals = (uint32_t*)malloc(map->cap * sizeof(uint32_t));
	if(!map->vals)
	{
		free(map->keys);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	memset(map->keys, UINT8_MAX, map->cap * sizeof(QOBJuvec3));

	return QOBJ_SUCCESS;
}

void qobj_hashmap_free(QOBJvertexHashmap map)
{
	free(map.keys);
	free(map.vals);
}

inline size_t qobj_hashmap_hash(QOBJuvec3 key)
{
	return 12637 * key.v[0] + 16369 * key.v[1] + 20749 * key.v[2];
}

QOBJerror qobj_hashmap_get_or_add(QOBJvertexHashmap* map, QOBJuvec3 key, uint32_t* val)
{
	//get hash:
	size_t hash = qobj_hashmap_hash(key) % map->cap;

	//linear probing:
	int32_t found = 0;
	while(map->keys[hash].v[0] != UINT32_MAX)
	{		
		if(map->keys[hash].v[0] == key.v[0] && map->keys[hash].v[1] == key.v[1] && map->keys[hash].v[2] == key.v[2])
		{
			found = 1;
			break;
		}

		hash++;
		hash %= map->cap;
	}

	if(found)
		*val = map->vals[hash];
	else
	{
		map->keys[hash] = key;
		map->vals[hash] = *val;
		map->size++;
	}

	//resize and rehash if needed:
	if(map->size >= map->cap / 2)
	{
		size_t oldCap = map->cap;
		map->cap *= 2;

		QOBJuvec3* newKeys = (QOBJuvec3*)malloc(map->cap * sizeof(QOBJuvec3));
		if(!newKeys)
			return QOBJ_ERROR_OUT_OF_MEM;
		uint32_t* newVals = (uint32_t*)malloc(map->cap * sizeof(uint32_t));
		if(!newVals)
		{
			free(newKeys);
			return QOBJ_ERROR_OUT_OF_MEM;
		}

		memset(newKeys, UINT8_MAX, map->cap * sizeof(QOBJvec3));

		for(uint32_t i = 0; i < oldCap; i++)
		{
			if(map->keys[i].v[0] == UINT32_MAX)
				continue;

			size_t newHash = qobj_hashmap_hash(map->keys[i]) % map->cap;
			newKeys[newHash] = map->keys[i];
			newVals[newHash] = map->vals[i];
		}

		free(map->keys);
		free(map->vals);
		map->keys = newKeys;
		map->vals = newVals;
	}

	return QOBJ_SUCCESS;
}

#define QOBJ_MAX_TOKEN_LEN 128

//----------------------------------------------------------------------//
//MESH FUNCTIONS

QOBJerror qobj_mesh_create(QOBJmesh* mesh, uint32_t materialIdx)
{
	mesh->vertexCap = 32;
	mesh->indexCap  = 32;
	mesh->numVertices = 0;
	mesh->numIndices  = 0;

	mesh->vertices = (QOBJvertex*)malloc(mesh->vertexCap * sizeof(QOBJvertex));
	if(!mesh->vertices)
		return QOBJ_ERROR_OUT_OF_MEM;

	mesh->indices = (uint32_t*)malloc(mesh->indexCap * sizeof(uint32_t));
	if(!mesh->indices)
	{
		free(mesh->vertices);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	mesh->materialIdx = materialIdx;

	return QOBJ_SUCCESS;
}

void qobj_mesh_free(QOBJmesh mesh)
{
	free(mesh.vertices);
	free(mesh.indices);
}

//----------------------------------------------------------------------//
//HELPER FUNCTIONS:

inline void qobj_next_token(FILE* fptr, char* token, char* endCh)
{
	char curCh;
	size_t curLen = 0;

	while(1)
	{
		if(curLen >= QOBJ_MAX_TOKEN_LEN)
		{
			curLen--;
			break;
		}

		curCh = fgetc(fptr);

		if(curCh == ' ' || curCh == '\n' || curCh == EOF)
			break;

		token[curLen++] = curCh;
	}

	token[curLen] = '\0';
	*endCh = curCh;
}

inline void qobj_fgets(FILE* fptr, char* token, char* endCh)
{
	fgets(token, QOBJ_MAX_TOKEN_LEN, fptr);

	size_t last = strlen(token) - 1;
	if(token[last] == '\n')
	{
		*endCh = token[last];
		token[last] = '\0';
	}
	else
		*endCh = EOF;
}

inline QOBJerror qobj_maybe_resize_buffer(void** buffer, size_t elemSize, size_t numElems, size_t* elemCap)
{
	if(numElems < *elemCap)
		return QOBJ_SUCCESS;
	
	*elemCap *= 2;
	void* newBuffer = realloc(*buffer, *elemCap * elemSize);
	if(!newBuffer)
		return QOBJ_ERROR_OUT_OF_MEM;
	*buffer = newBuffer;

	return QOBJ_SUCCESS;
}

//----------------------------------------------------------------------//
//MTL FUNCTIONS:

QOBJmaterial qobj_default_material()
{
	QOBJmaterial result = {0};

	result.opacity = 1.0f;
	result.specularExp = 1.0f;
	result.refractionIndex = 1.0f;

	return result;
}

void qobj_mtl_free(size_t numMaterials, QOBJmaterial* materials)
{
	for(uint32_t i = 0; i < numMaterials; i++)
	{
		if(materials[i].name)
			free(materials[i].name);

		if(materials[i].ambientMapPath)
			free(materials[i].ambientMapPath);
		if(materials[i].diffuseMapPath)
			free(materials[i].diffuseMapPath);
		if(materials[i].specularMapPath)
			free(materials[i].specularMapPath);
		if(materials[i].normalMapPath)
			free(materials[i].normalMapPath);
	}

	if(materials)
		free(materials);
}

QOBJerror qobj_mtl_load(const char* path, size_t* numMaterials, QOBJmaterial** materials)
{
	//ensure file is valid and able to be opened:
	size_t pathLen = strlen(path);
	if(pathLen < 4 || strcmp(&path[pathLen - 4], ".mtl") != 0)
		return QOBJ_ERROR_INVALID_FILE;

	FILE* fptr = fopen(path, "r");
	if(!fptr)
		return QOBJ_ERROR_IO;

	QOBJerror errorCode = QOBJ_SUCCESS;

	//allocate memory:
	*materials = (QOBJmaterial*)malloc(sizeof(QOBJmaterial));
	*numMaterials = 0;
	size_t curMaterial = 0;

	if(!*materials)
	{
		errorCode = QOBJ_ERROR_OUT_OF_MEM;
		goto cleanup;
	}

	//main loop:
	char curToken[QOBJ_MAX_TOKEN_LEN];
	char curTokenEnd;

	while(1)
	{
		qobj_next_token(fptr, curToken, &curTokenEnd);

		if(curTokenEnd == EOF)
			break;

		if(curToken[0] == '\0')
			continue;

		if(curToken[0] == '#' || strcmp(curToken, "illum") == 0 ||
		   strcmp(curToken, "Tf") == 0) //comments / ignored commands
		{
			if(curTokenEnd == ' ')
				qobj_fgets(fptr, curToken, &curTokenEnd);
		}
		else if(strcmp(curToken, "newmtl") == 0)
		{
			qobj_fgets(fptr, curToken, &curTokenEnd);

			curMaterial = *numMaterials;
			(*numMaterials)++;
			*materials = (QOBJmaterial*)realloc(*materials, *numMaterials * sizeof(QOBJmaterial));
			if(!*materials)
			{
				errorCode = QOBJ_ERROR_OUT_OF_MEM;
				goto cleanup;
			}

			(*materials)[curMaterial] = qobj_default_material();
			(*materials)[curMaterial].name = (char*)malloc(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			strcpy((*materials)[curMaterial].name, curToken);
		}
		else if(strcmp(curToken, "Ka") == 0)
		{
			QOBJvec3 col;
			fscanf(fptr, "%f %f %f\n", &col.v[0], &col.v[1], &col.v[2]);

			(*materials)[curMaterial].ambientColor = col;
		}
		else if(strcmp(curToken, "Kd") == 0)
		{
			QOBJvec3 col;
			fscanf(fptr, "%f %f %f\n", &col.v[0], &col.v[1], &col.v[2]);

			(*materials)[curMaterial].diffuseColor = col;
		}
		else if(strcmp(curToken, "Ks") == 0)
		{
			QOBJvec3 col;
			fscanf(fptr, "%f %f %f\n", &col.v[0], &col.v[1], &col.v[2]);

			(*materials)[curMaterial].specularColor = col;
		}
		else if(strcmp(curToken, "d") == 0)
		{
			float opacity;
			fscanf(fptr, "%f\n", &opacity);

			(*materials)[curMaterial].opacity = opacity;
		}
		else if(strcmp(curToken, "Ns") == 0)
		{
			float specularExp;
			fscanf(fptr, "%f\n", &specularExp);

			(*materials)[curMaterial].specularExp = specularExp;
		}
		else if(strcmp(curToken, "Ni") == 0)
		{
			float refractionIndex;
			fscanf(fptr, "%f\n", &refractionIndex);

			(*materials)[curMaterial].refractionIndex = refractionIndex;
		}
		else if(strcmp(curToken, "map_Ka") == 0)
		{
			char* mapPath = (char*)malloc(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].ambientMapPath = mapPath;
		}
		else if(strcmp(curToken, "map_Kd") == 0)
		{
			char* mapPath = (char*)malloc(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].diffuseMapPath = mapPath;
		}
		else if(strcmp(curToken, "map_Ks") == 0)
		{
			char* mapPath = (char*)malloc(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].specularMapPath = mapPath;
		}
		else if(strcmp(curToken, "map_Bump") == 0)
		{
			char* mapPath = (char*)malloc(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].normalMapPath = mapPath;
		}
	}

	cleanup: ;

	if(errorCode != QOBJ_SUCCESS)
	{
		qobj_mtl_free(*numMaterials, *materials);
		*numMaterials = 0;
	}

	fclose(fptr);
	return errorCode;	
}

//----------------------------------------------------------------------//
//OBJ FUNCTIONS:

QOBJerror qobj_load(const char* path, size_t* numMeshes, QOBJmesh** meshes, size_t* numMaterials, QOBJmaterial** materials)
{
	*numMeshes = 0;
	*meshes = NULL;
	*numMaterials = 0;
	*materials = NULL;

	//ensure file is valid and able to be opened:
	size_t pathLen = strlen(path);
	if(pathLen < 4 || strcmp(&path[pathLen - 4], ".obj") != 0)
		return QOBJ_ERROR_INVALID_FILE;

	FILE* fptr = fopen(path, "r");
	if(!fptr)
		return QOBJ_ERROR_IO;

	QOBJerror errorCode = QOBJ_SUCCESS;

	//allocate memory:
	size_t positionSize = 0 , normalSize = 0 , texCoordSize = 0;
	size_t positionCap  = 32, normalCap  = 32, texCoordCap  = 32;
	QOBJvec3* positions = (QOBJvec3*)malloc(positionCap * sizeof(QOBJvec3));
	QOBJvec3* normals   = (QOBJvec3*)malloc(normalCap   * sizeof(QOBJvec3));
	QOBJvec2* texCoords = (QOBJvec2*)malloc(texCoordCap * sizeof(QOBJvec2));

	*meshes = (QOBJmesh*)malloc(sizeof(QOBJmesh));
	QOBJvertexHashmap* meshVertexMaps = (QOBJvertexHashmap*)malloc(sizeof(QOBJvertexHashmap));
	*numMeshes = 0;
	size_t curMesh = 0;

	//ensure memory was properly allocated:
	if(!positions || !normals || !texCoords || !*meshes || !meshVertexMaps)
	{
		errorCode = QOBJ_ERROR_OUT_OF_MEM;
		goto cleanup;
	}

	//main loop:
	char curToken[QOBJ_MAX_TOKEN_LEN];
	char curTokenEnd;

	while(1)
	{
		qobj_next_token(fptr, curToken, &curTokenEnd);

		if(curTokenEnd == EOF)
			break;

		if(curToken[0] == '\0')
			continue;

		if(curToken[0] == '#'         || strcmp(curToken, "o") == 0 || 
		   strcmp(curToken, "g") == 0 || strcmp(curToken, "s") == 0) //comments / ignored commands
		{
			if(curTokenEnd == ' ')
				qobj_fgets(fptr, curToken, &curTokenEnd);
		}
		else if(strcmp(curToken, "v") == 0)
		{
			QOBJvec3 pos;
			fscanf(fptr, "%f %f %f\n", &pos.v[0], &pos.v[1], &pos.v[2]);

			positions[positionSize++] = pos;
			QOBJerror resizeError = qobj_maybe_resize_buffer((void**)&positions, sizeof(QOBJvec3), positionSize, &positionCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "vn") == 0)
		{
			QOBJvec3 normal;
			fscanf(fptr, "%f %f %f\n", &normal.v[0], &normal.v[1], &normal.v[2]);

			normals[normalSize++] = normal;
			QOBJerror resizeError = qobj_maybe_resize_buffer((void**)&normals, sizeof(QOBJvec3), normalSize, &normalCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "vt") == 0)
		{
			QOBJvec2 texCoord;
			fscanf(fptr, "%f %f\n", &texCoord.v[0], &texCoord.v[1]);

			texCoords[texCoordSize++] = texCoord;
			QOBJerror resizeError = qobj_maybe_resize_buffer((void**)&texCoords, sizeof(QOBJvec2), texCoordSize, &texCoordCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "f") == 0)
		{
			//if no material was selected (this always occurs first):
			if(curMesh >= *numMeshes)
			{
				curMesh = 0;
				*numMeshes = 1;

				QOBJerror meshCreateError = qobj_mesh_create(&(*meshes)[curMesh], UINT32_MAX); //default material
				if(meshCreateError != QOBJ_SUCCESS)
				{
					errorCode = meshCreateError;
					goto cleanup;
				}
			}

			uint32_t numVertices = 0;
			QOBJuvec3 vertices[6];

			while(numVertices < 4)
			{
				//read position:
				fscanf(fptr, "%d", &vertices[numVertices].v[0]);

				char nextCh = fgetc(fptr);
				if(nextCh == ' ')
					continue;
				else if(nextCh == '\n')
					break;

				//read normal (if no texture coordinates exist):
				nextCh = fgetc(fptr);
				if(nextCh == '/')
					fscanf(fptr, "%d", &vertices[numVertices].v[2]);
				else
					ungetc(nextCh, fptr);
				
				//read texture coordinates:
				fscanf(fptr, "%d", &vertices[numVertices].v[1]);
					
				//read normal (if nexture coordinates exist):
				nextCh = fgetc(fptr);
				if(nextCh == '/')
					fscanf(fptr, "%d", &vertices[numVertices].v[2]);
				else
					ungetc(nextCh, fptr);
				
				//break if at end
				nextCh = fgetc(fptr);
				if (nextCh == '\n' || nextCh == EOF)
					break;

				numVertices++;
			}
			numVertices++;

			//split quad into 2 triangles:
			if(numVertices == 4)
			{
				QOBJuvec3 temp = vertices[3];
				vertices[3] = vertices[0];
				vertices[4] = vertices[2];
				vertices[5] = temp;

				numVertices = 6;
			}

			QOBJmesh* mesh = &(*meshes)[curMesh];
			QOBJvertexHashmap* map = &meshVertexMaps[curMesh]; 

			//resize if needed:
			qobj_maybe_resize_buffer((void**)&mesh->indices, sizeof(uint32_t), mesh->numIndices + numVertices, &mesh->indexCap);
			qobj_maybe_resize_buffer((void**)&mesh->vertices, sizeof(QOBJvertex), mesh->numVertices + numVertices, &mesh->vertexCap); //potentially wasteful since we dont necessarily add each vertex

			//add vertices if not already indexed; add indices:
			for(uint32_t i = 0; i < numVertices; i++)
			{
				uint32_t indexToAdd = (uint32_t)mesh->numVertices;
				qobj_hashmap_get_or_add(map, vertices[i], &indexToAdd);

				if(indexToAdd == (uint32_t)mesh->numVertices)
				{
					QOBJvertex vertex;
					vertex.pos      = positions[vertices[i].v[0] - 1];
					vertex.texCoord = texCoords[vertices[i].v[1] - 1];
					vertex.normal   = normals  [vertices[i].v[2] - 1];

					mesh->vertices[mesh->numVertices++] = vertex;
				}

				mesh->indices[mesh->numIndices++] = indexToAdd;
			}
		}
		else if(strcmp(curToken, "usemtl") == 0)
		{
			qobj_fgets(fptr, curToken, &curTokenEnd);

			//find material:
			uint32_t materialIdx = UINT32_MAX; //default material
			for(uint32_t i = 0; i < *numMaterials; i++)
				if(strcmp((*materials)[i].name, curToken) == 0)
				{
					materialIdx = i;
					break;
				}

			//search for existing mesh with same material:
			for(curMesh = 0; curMesh < *numMeshes; curMesh++)
				if((*meshes)[curMesh].materialIdx == materialIdx)
					break;

			//add new mesh if needed:
			if(curMesh >= *numMeshes)
			{
				(*numMeshes)++;
				*meshes = (QOBJmesh*)realloc(*meshes, *numMeshes * sizeof(QOBJmesh));
				meshVertexMaps = (QOBJvertexHashmap*)realloc(meshVertexMaps, *numMeshes * sizeof(QOBJvertexHashmap));

				if(!*meshes || !meshVertexMaps)
				{
					errorCode = QOBJ_ERROR_OUT_OF_MEM;
					goto cleanup;
				}

				QOBJerror meshCreateError = qobj_mesh_create(&(*meshes)[curMesh], materialIdx);
				if(meshCreateError != QOBJ_SUCCESS)
				{
					errorCode = meshCreateError;
					goto cleanup;
				}

				meshCreateError = qobj_hashmap_create(&meshVertexMaps[curMesh]);
				if(meshCreateError != QOBJ_SUCCESS)
				{
					errorCode = meshCreateError;
					goto cleanup;
				}
			}
		}
		else if(strcmp(curToken, "mtllib") == 0)
		{
			qobj_fgets(fptr, curToken, &curTokenEnd);
			
			//get directory of current file:
			uint32_t i = 0;
			uint32_t lastSlash = 0;

			while(path[i])
			{
				if(path[i] == '/' || path[i] == '\\')
					lastSlash = i;

				i++;
			}

			char mtlPath[QOBJ_MAX_TOKEN_LEN];
			strncpy(mtlPath, path, lastSlash + 1);
			mtlPath[lastSlash + 1] = '\0';
			strcat(mtlPath, curToken);

			QOBJerror mtlError = qobj_mtl_load(mtlPath, numMaterials, materials);
			if(mtlError != QOBJ_SUCCESS) //TODO: decide if we should actually throw an error on failed mtl file load since materials arent fully necessary
			{
				errorCode = mtlError;
				goto cleanup;
			}
		}
		else
		{
			errorCode = QOBJ_ERROR_UNSUPPORTED_DATA_TYPE;
			goto cleanup;
		}
	}

	//add default material to list if needed:
	uint8_t addedDefault = 0;
	for(uint32_t i = 0; i < *numMeshes; i++)
		if((*meshes)[i].materialIdx == UINT32_MAX)
		{
			if(addedDefault)
			{
				(*meshes)[i].materialIdx = *numMaterials - 1;
				continue;
			}

			(*numMaterials)++;
			*materials = (QOBJmaterial*)realloc(*materials, *numMaterials * sizeof(QOBJmaterial));
			if(!*materials)
			{
				errorCode = QOBJ_ERROR_OUT_OF_MEM;
				goto cleanup;
			}

			(*materials)[*numMaterials - 1] = qobj_default_material();
		}

	cleanup: ;

	for(uint32_t i = 0; i < *numMeshes; i++)
		qobj_hashmap_free(meshVertexMaps[i]);
	if(meshVertexMaps)
		free(meshVertexMaps);

	if(errorCode != QOBJ_SUCCESS)
	{
		qobj_free(*numMeshes, *meshes, *numMaterials, *materials);
		*numMeshes = 0;
	}

	if(positions)
		free(positions);
	if(normals)
		free(normals);
	if(texCoords)
		free(texCoords);

	fclose(fptr);
	return errorCode;
}

void qobj_free(size_t numMeshes, QOBJmesh* meshes, size_t numMaterials, QOBJmaterial* materials)
{
	for(uint32_t i = 0; i < numMeshes; i++)
		qobj_mesh_free(meshes[i]);

	if(meshes)
		free(meshes);

	qobj_mtl_free(numMaterials, materials);
}

//----------------------------------------------------------------------//

#endif //#ifdef QOBJ_IMPLEMENTATION

#ifdef __cplusplus
} //extern "C"
#endif

#endif //QOBJ_H
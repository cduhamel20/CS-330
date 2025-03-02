///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	// initialize the texture collection
	for (int i = 0; i < 16; i++)
{
	m_textureIDs[i].tag = "/0";
	m_textureIDs[i].ID = -1;
}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
}
/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

//Method to set the texturs into the scence
void SceneManager::LoadSceneTextures()
{

	{
		bool bReturn = false;

		bReturn = CreateGLTexture("textures/Desk texture.jpg", "DeskTexture");
		bReturn = CreateGLTexture("textures/BlackBezzle.jpg", "BlackBezzle");
		bReturn = CreateGLTexture("textures/Steel.jpg", "Steel");
		bReturn = CreateGLTexture("textures/coffeecuptexture.jpg", "CupTexture");


	}


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

//Creating the material for the light to reflect

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	goldMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	goldMaterial.shininess = 60.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 90.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

}

//setting up the lights for the scene
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light 
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -1.0f, 0.0f); 
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.1f, 0.1f, 0.1f);   
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.4f, 0.4f, 0.4f);   
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 1.0f, 1.0f); 
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1 
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 55.0f, 0.0f); 
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.1f, 0.1f); 
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 1.0f, 1.0f); 
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 1.0f, 1.0f); 
	m_pShaderManager->setVec3Value("pointLights[0].attenuation", 1.0f, 0.1f, 0.05f); 
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", -15.0f, 55.0f, 0.0f); 
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.1f, 0.1f); 
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 1.0f, 1.0f, 1.0f); 
	m_pShaderManager->setVec3Value("pointLights[1].attenuation", 1.0f, 0.1f, 0.05f); 
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Point light 3 
	m_pShaderManager->setVec3Value("pointLights[2].position", 0.0f, 55.0f, -5.0f); 
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.1f, 0.1f, 0.1f); 
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 1.0f, 1.0f, 1.0f); 
	m_pShaderManager->setVec3Value("pointLights[2].attenuation", 1.0f, 0.1f, 0.05f); 
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// load the textures for the 3D scene
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	SetupSceneLights();

	DefineObjectMaterials();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 2.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("DeskTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");
	//SetShaderColor(1, 1, 1, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Settingup the back planeto create the backdrop of the of the scene
	SetTransformations(
		scaleXYZ = glm::vec3(30.0f, 2.0f, 15.0),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 15.f, -15.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.9, 0.9, 0.9, 1);
	m_basicMeshes->DrawPlaneMesh();

	//this is the first layer of the computer monitor (black bezzle) 
	SetTransformations(
		scaleXYZ = glm::vec3(18.0f, 0.5f, 11.0f),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 8.0f, -7.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	//texture loading
	SetShaderTexture("BlackBezzle");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0, 1.0);
	
	//SetShaderColor(0.0, 0.0, 0.0, 1);
	m_basicMeshes->DrawBoxMesh();

	//This is the inner white part of the monitor
	SetTransformations(
		scaleXYZ = glm::vec3(16.0f, 0.7f, 9.0f),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 8.0f, -7.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1, 1, 1, 1); 
	
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(
		scaleXYZ = glm::vec3(18.0f, 0.7f, 1.0f),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 2.4f, -7.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	//Texture
	SetShaderTexture("Steel");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0, 1.0);
	
	//SetShaderColor(0.55, 0.55, 0.55, 1);
	m_basicMeshes->DrawBoxMesh();

	//this box is to make the back of the monitor black
	SetTransformations(
		scaleXYZ = glm::vec3(18.0f, 0.5f, 11.0f),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 8.0f, -7.5f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0, 0.0, 0.0, 1);
	m_basicMeshes->DrawBoxMesh();

	//this is the stand for the monitor
	SetTransformations(
		scaleXYZ = glm::vec3(5.0f, 0.5f, 6.0f),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 0.0f, -7.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	SetShaderTexture("Steel");
	SetTextureUVScale(1.0, 1.0);
	
	//SetShaderColor(0.55, 0.55, 0.55, 1);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(
		scaleXYZ = glm::vec3(8.0f, 4.5f, 1.5f),
		XrotationDegrees = 90,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(0.0f, 0.0f, -6.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	//Texture loading
	SetShaderTexture("Steel");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0, 1.0);
	
	//SetShaderColor(0.55, 0.55, 0.55, 1);
	m_basicMeshes->DrawBoxMesh();

	// Tapered Cup Body
	SetTransformations(
		scaleXYZ = glm::vec3(1.8f, 2.8f, 1.8f), 
		XrotationDegrees = 180,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(-8.7f, 3.0f, -4.6f));

	//Texture
	SetShaderTexture("CupTexture");
	SetShaderMaterial("glass");
	SetTextureUVScale(1.0, 1.0);
	
	//SetShaderColor(0.2, 0.2, 0.2, 1);
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Cup Handle 
	SetTransformations(
		scaleXYZ = glm::vec3(0.8f, 0.8f, 0.3f), 
		XrotationDegrees = 0,  
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(-6.9f, 1.3f, -4.6f));

	//Texture
	SetShaderTexture("CupTexture");
	SetShaderMaterial("glass");
	SetTextureUVScale(0.0, 0.0);
	
	//SetShaderColor(0.2, 0.2, 0.2, 1);
	m_basicMeshes->DrawTorusMesh();

	//Keyboard
	SetTransformations(
		scaleXYZ = glm::vec3(11.8f, 0.8f, 3.8f),
		XrotationDegrees = 0,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(-2.2f,0.0f, 0.0f));
	

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//color
	SetShaderColor(1, 1, 1, 1);
	m_basicMeshes->DrawBoxMesh();

	//Texture
	//SetShaderTexture("CupTexture");
	//SetShaderMaterial("glass");
	//SetTextureUVScale(1.0, 1.0);


	//mouse
	SetTransformations(
		scaleXYZ = glm::vec3(1.6f, 1.0f, 0.2),
		XrotationDegrees = 0,
		YrotationDegrees = 90,
		ZrotationDegrees = 90,
		positionXYZ = glm::vec3(6.2f, 0.3f, 0.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//color
	SetShaderColor(1, 1, 1, 1);
	m_basicMeshes->DrawSphereMesh();


	//pencil cup
	SetTransformations(
		scaleXYZ = glm::vec3(1.8f, 2.8f, 1.8f),
		XrotationDegrees =180,
		YrotationDegrees = 0,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(11.2f, 2.8f, -5.3f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//color
	SetShaderColor(1, 1, 1, 1);
	m_basicMeshes->DrawTaperedCylinderMesh();


	// Pencil 1
	SetTransformations(
		scaleXYZ = glm::vec3(0.2f, 3.5f, 0.2f), 
		XrotationDegrees = 5,  
		YrotationDegrees = 15,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(11.2f, 1.0f, -5.3f));  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0, 0, 0, 1); 
	m_basicMeshes->DrawCylinderMesh();  

	// Pencil 2
	SetTransformations(
		scaleXYZ = glm::vec3(0.2f, 3.8f, 0.2f),
		XrotationDegrees = -13,  
		YrotationDegrees = -10,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(10.8f, 1.3f, -5.2f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	m_basicMeshes->DrawCylinderMesh();

	// Pencil 3
	SetTransformations(
		scaleXYZ = glm::vec3(0.2f, 3.2f, 0.2f),
		XrotationDegrees = 7,
		YrotationDegrees = -5,
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(10.1f, 1.9f, -5.4f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0, 0, 0, 1);
	m_basicMeshes->DrawCylinderMesh();

	// Book 1 
	SetTransformations(
		scaleXYZ = glm::vec3(3.5f, 0.5f, 2.5f),  
		XrotationDegrees = 0,
		YrotationDegrees = -5,  
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(-13.0f, 0.25f, -5.0f));  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	m_basicMeshes->DrawBoxMesh();

	// Book 2 
	SetTransformations(
		scaleXYZ = glm::vec3(3.3f, 0.4f, 2.4f),  
		XrotationDegrees = 0,
		YrotationDegrees = 3,  
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(-12.9f, 0.75f, -5.2f));  

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1, 1, 1, 1);  
	m_basicMeshes->DrawBoxMesh();

	// Book 3 
	SetTransformations(
		scaleXYZ = glm::vec3(3.2f, 0.3f, 2.3f),
		XrotationDegrees = 0,
		YrotationDegrees = -7,  
		ZrotationDegrees = 0,
		positionXYZ = glm::vec3(-13.2f, 1.1f, -4.8f)); 

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1);  
	m_basicMeshes->DrawBoxMesh();



	/****************************************************************/
}

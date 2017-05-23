#pragma once
#include <fbxsdk.h>
#include <Windows.h>
#include <vector>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <d3d11.h>

#ifdef FBXDLL_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
using namespace DirectX;
using namespace std;

namespace DllExport
{
	struct PNUVertex
	{
		XMFLOAT3 mPosition;
		XMFLOAT3 mNormal;
		XMFLOAT2 mUV;
	};

	struct Joint
	{
		FbxNode* mNode;
		string mName;
		FbxAMatrix mGlobalBindposeInverse;
		int mParentIndex;
	};

	struct Skeleton
	{
		vector<Joint> mJoints;
	};

	struct CtrlPoint
	{
		XMFLOAT3 mPosition;
		//vector<BlendingIndexWeightPair> mBlendingInfo;

		//CtrlPoint()
		//{
		//	mBlendingInfo.reserve(4);
		//}
	};

	struct Triangle
	{
		vector<unsigned int> mIndices;
		string mMaterialName;
		unsigned int mMaterialIndex;

		bool operator<(const Triangle& rhs)
		{
			return mMaterialIndex < rhs.mMaterialIndex;
		}
	};

	struct JointMatrix
	{
		float global_xform[16];
		int mParentIndex;
		double time;
	};

	struct Keyframe
	{
		double time;
		vector<FbxAMatrix> joints;
	};

	struct anim_clip
	{
		double duration;
		vector<Keyframe*> frames;
	};

	struct vert_pos_skinned
	{
		vector<float> pos[3];
		int joints[4];
		vector<float> weights[4];
	};

	class DLLEXPORT Export
	{
	public:
		Export();
		bool Initialize();
		bool LoadScene(const char* inFileName);
		FbxScene* getScene() { return mScene; }
		vector<PNUVertex> getVertices() { return mVerts; }
		void ProcessSkeletonHierarchy(FbxNode* inRootNode);
		void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex);
		void ProcessControlPoints(FbxNode* inNode);
		void ProcessMesh(FbxNode* inNode);
		FbxAMatrix Export::GetGeometryTransforms(FbxNode * inNode);
		unsigned int Export::FindJointIndexUsingName(const string & inJointName);
		void Export::ProcessJoints(FbxNode * inNode);
		void Export::ProcessAnimations(FbxNode * inNode);
		std::vector<PNUVertex> LoadFBX(std::vector<PNUVertex> outVerts, const char* file);
		std::vector<JointMatrix> GetJoints(std::vector<JointMatrix> outJoints, const char* file);
		std::vector<vector<JointMatrix>> GetKeyframes(std::vector<vector<JointMatrix>> outFrames, const char * file);
		std::vector<vert_pos_skinned> GetWeights(std::vector<vert_pos_skinned> outJoints, const char* file);
		void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inTextureUVIndex, int inUVLayer, XMFLOAT2& outUV);
		void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal);
		void ReadBinormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outBinormal);
		void ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outTangent);
		Skeleton getSkelton() { return mSkeleton; }

	private:
		FbxManager* mFBXMan;
		FbxScene* mScene;
		Skeleton mSkeleton;
		unsigned int mTriCount;
		vector<Triangle> mTris;
		vector<PNUVertex> mVerts;
		unordered_map<unsigned int, CtrlPoint*> mControlPoints;
		vector<XMFLOAT3> m_ControlVectors;
		string inputFilePath;
		string outputFilePath;
		anim_clip animation;
	};
}


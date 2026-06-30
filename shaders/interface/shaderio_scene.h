//==============================================================================
// 鏂囦欢锛歴haders/interface/shaderio_scene.h
// 妯″潡瀹氫綅锛欳PU 涓?GPU 鍏变韩甯冨眬鏂囦欢锛屽畾涔夌潃鑹插櫒鍜?C++ 鍏卞悓鐞嗚В鐨勬暟鎹粨鏋勩€佸父閲忓拰璁块棶绾﹀畾銆?// 鏁版嵁娴侊細CPU 渚у～鍏呰繖浜涚粨鏋勶紝GPU 渚ф寜瀹屽叏鐩稿悓鐨勫唴瀛樺竷灞€璇诲彇鍜屽啓鍥炪€?// 鏂规硶璇存槑锛氬叡浜竷灞€鏄紓鏋勭郴缁熺殑 ABI锛屼换浣曞瓧娈甸『搴忋€佸榻愬拰浣嶅煙鍙樺寲閮戒細褰卞搷涓や晶瑙ｉ噴涓€鑷存€с€?// 姝ｇ‘鎬х害鏉燂細缁撴瀯瀵归綈銆佹爣閲忓竷灞€鍜?缂撳啿 reference 绫诲瀷蹇呴』涓?Vulkan/GLSL 缂栬瘧閫夐」涓€鑷淬€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?GPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ュ叡浜竷灞€銆佸墧闄ゃ€佺潃鑹叉垨闃舵闂村鐢ㄧ殑鐫€鑹插櫒鐗囨銆?// 杩欎簺 include 鍏卞悓鍐冲畾鏈枃浠惰兘璁块棶鐨勭粨鏋勫竷灞€銆佹暟瀛﹁緟鍔╁嚱鏁板拰缂栬瘧鏈熷畯銆?#include "shaderio_core.h"
#include "shaderio_core.h"
#ifndef _SHADERIO_SCENE_H_


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define _SHADERIO_SCENE_H_
#define _SHADERIO_SCENE_H_
#ifdef __cplusplus


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace shaderio {
namespace shaderio {
using namespace glm;


// 鏋氫妇锛欳lusterAttributeBits銆傞泦涓畾涔夋湰妯″潡鍙€夋ā寮忔垨鐘舵€佸€硷紝閬垮厤璋冪敤鐐逛娇鐢ㄨ８鏁存暟銆?// 璁捐鎰忓浘锛氭妸瀹為獙寮€鍏炽€佹覆鏌撴ā寮忔垨闃舵缂栧彿鏄惧紡鍛藉悕锛屼娇閰嶇疆鏂囦欢銆乁I 鍜屼唬鐮佽矾寰勫彲浠ヤ簰鐩稿搴斻€?// 浣跨敤绾︽潫锛氭柊澧炴灇涓惧€兼椂闇€瑕佸悓姝?UI 鏂囨湰銆佸弬鏁拌В鏋愬拰鐩稿叧 switch 鍒嗘敮銆?enum ClusterAttributeBits
enum ClusterAttributeBits
{
  CLUSTER_ATTRIBUTE_VERTEX_NORMAL           = 1,
  CLUSTER_ATTRIBUTE_VERTEX_TANGENT          = 2,
  CLUSTER_ATTRIBUTE_VERTEX_TEX_0            = 4,
  CLUSTER_ATTRIBUTE_VERTEX_TEX_1            = 8,
  CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_TEX_0 = 32,
  CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_TEX_1 = 64,
  CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_POS   = 128,
};

#else


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_VERTEX_NORMAL 1
#define CLUSTER_ATTRIBUTE_VERTEX_NORMAL 1


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_VERTEX_TANGENT 2
#define CLUSTER_ATTRIBUTE_VERTEX_TANGENT 2


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_VERTEX_TEX_0 4
#define CLUSTER_ATTRIBUTE_VERTEX_TEX_0 4


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_VERTEX_TEX_1 8
#define CLUSTER_ATTRIBUTE_VERTEX_TEX_1 8


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_TEX_0 32
#define CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_TEX_0 32


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_TEX_1 64
#define CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_TEX_1 64


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_POS 128
#define CLUSTER_ATTRIBUTE_COMPRESSED_VERTEX_POS 128

#ifndef CLUSTER_VERTEX_COUNT


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_VERTEX_COUNT 32
#define CLUSTER_VERTEX_COUNT 32
#endif

#ifndef CLUSTER_TRIANGLE_COUNT


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define CLUSTER_TRIANGLE_COUNT 32
#define CLUSTER_TRIANGLE_COUNT 32
#endif

#endif


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define SHADERIO_ORIGINAL_MESH_GROUP 0xffffffffu
#define SHADERIO_ORIGINAL_MESH_GROUP 0xffffffffu


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define SHADERIO_MAX_LOD_LEVELS 32
#define SHADERIO_MAX_LOD_LEVELS 32


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define SHADERIO_MAX_NODE_CHILDREN 32
#define SHADERIO_MAX_NODE_CHILDREN 32


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define SHADERIO_MAX_GROUP_CLUSTERS 128
#define SHADERIO_MAX_GROUP_CLUSTERS 128


#define SHADERIO_INVALID_ASSEMBLY 0xffffffffu


#define SHADERIO_ASSEMBLY_VISIBLE_BIT 1u
#define SHADERIO_ASSEMBLY_LOD_COARSE_BIT 2u

#define SEMANTIC_LOD_VALID_BIT 1u
#define SEMANTIC_LOD_ALLOW_CULL_BIT 2u
#define SEMANTIC_LOD_AGGRESSIVE_BIT 4u
#define SEMANTIC_LOD_PRESERVE_BIT 8u
#define SEMANTIC_LOD_LOW_CONF_BIT 16u
#define SEMANTIC_LOD_PRIORITY_SHIFT 8u
#define SEMANTIC_LOD_PRIORITY_MASK 0xFu


// 缁撴瀯锛欱Box銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct BBox
struct BBox
{
  vec3 lo;
  vec3 hi;

  float shortestEdge;
  float longestEdge;
};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE(BBox_in, BBox, readonly, 16);
BUFFER_REF_DECLARE(BBox_in, BBox, readonly, 16);


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_ARRAY(BBoxes_in, BBox, readonly, 16);
BUFFER_REF_DECLARE_ARRAY(BBoxes_in, BBox, readonly, 16);


struct AssemblyNode
{
  BBox     bbox;
  uint32_t firstInstance;
  uint32_t instanceCount;
  uint32_t childCount;
  uint32_t depth;
  uint32_t templateID;
  uint32_t templateInstanceID;
  uint32_t _pad0;
};


BUFFER_REF_DECLARE_ARRAY(AssemblyNodes_in, AssemblyNode, readonly, 16);


struct AssemblyState
{
  uint32_t flags;
  float    screenPixels;
  float    errorOverDistance;
  uint32_t reserved;
};


BUFFER_REF_DECLARE_ARRAY(AssemblyStates_inout, AssemblyState, , 4);


// 缁撴瀯锛欳luster銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct Cluster
struct Cluster
{
  uint8_t triangleCountMinusOne;
  uint8_t vertexCountMinusOne;
  uint8_t lodLevel;
  uint8_t groupChildIndex;

  uint8_t  attributeBits;
  uint8_t  localMaterialID;
  uint16_t reserved;


  uint32_t vertices;
  uint32_t indices;

};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE(Cluster_in, Cluster, , 16);
BUFFER_REF_DECLARE(Cluster_in, Cluster, , 16);


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_ARRAY(Clusters_inout, Cluster, , 16);
BUFFER_REF_DECLARE_ARRAY(Clusters_inout, Cluster, , 16);


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_SIZE(Cluster_size, Cluster, 16);
BUFFER_REF_DECLARE_SIZE(Cluster_size, Cluster, 16);

#ifndef __cplusplus


// 鍑芥暟锛欳luster_getVertexPositions銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?vec3s_in Cluster_getVertexPositions(Cluster_in cluster)
vec3s_in Cluster_getVertexPositions(Cluster_in cluster)
{
  return vec3s_in(uint64_t(cluster) + cluster.d.vertices);
}


// 鍑芥暟锛欳luster_getVertexNormals銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?uint32s_in Cluster_getVertexNormals(Cluster_in cluster)
uint32s_in Cluster_getVertexNormals(Cluster_in cluster)
{
  return uint32s_in(uint64_t(cluster) + (cluster.d.vertices + 4 * 3 * (cluster.d.vertexCountMinusOne + 1)));
}


// 鍑芥暟锛欳luster_getVertexTexCoords銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?vec2s_in Cluster_getVertexTexCoords(Cluster_in cluster)
vec2s_in Cluster_getVertexTexCoords(Cluster_in cluster)
{

  uint32_t elems = (cluster.d.attributeBits & CLUSTER_ATTRIBUTE_VERTEX_NORMAL) == 0 ? 3 : 4;
  return vec2s_in(uint64_t(cluster) + (((cluster.d.vertices + 4 * elems * (cluster.d.vertexCountMinusOne + 1)) + 7) & ~7));
}


// 鍑芥暟锛欳luster_getTriangleIndices銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?uint8s_in Cluster_getTriangleIndices(Cluster_in cluster)
uint8s_in Cluster_getTriangleIndices(Cluster_in cluster)
{
  return uint8s_in(uint64_t(cluster) + cluster.d.indices);
}


// 鍑芥暟锛欳luster_getTriangleMaterials銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?uint8s_in Cluster_getTriangleMaterials(Cluster_in cluster)
uint8s_in Cluster_getTriangleMaterials(Cluster_in cluster)
{
  return uint8s_in(uint64_t(cluster) + (cluster.d.indices + 3 * (cluster.d.triangleCountMinusOne + 1)));
}
#endif


// 缁撴瀯锛歍raversalMetric銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct TraversalMetric
struct TraversalMetric
{


  float boundingSphereX;
  float boundingSphereY;
  float boundingSphereZ;
  float boundingSphereRadius;
  float maxQuadricError;
};


// 缁撴瀯锛欸roup銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct Group
struct Group
{


  uint32_t residentID;
  uint32_t clusterResidentID;

  uint16_t lodLevel;
  uint16_t clusterCount;

  TraversalMetric traversalMetric;
};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE(Group_in, Group, , 16);
BUFFER_REF_DECLARE(Group_in, Group, , 16);


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_ARRAY(Groups_in, Group, , 16);
BUFFER_REF_DECLARE_ARRAY(Groups_in, Group, , 16);


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_SIZE(Group_size, Group, 32);
BUFFER_REF_DECLARE_SIZE(Group_size, Group, 32);

#ifndef __cplusplus


// 鍑芥暟锛欸roup_getGeneratingGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?uint Group_getGeneratingGroup(Group_in group, uint clusterIndex)
uint Group_getGeneratingGroup(Group_in group, uint clusterIndex)
{
  return uint32s_in(uint64_t(group) + uint32_t(Group_size + Cluster_size * group.d.clusterCount)).d[clusterIndex];
}


// 鍑芥暟锛欸roup_getClusterBBox銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?BBox Group_getClusterBBox(Group_in group, uint clusterIndex)
BBox Group_getClusterBBox(Group_in group, uint clusterIndex)
{
  return BBoxes_in(uint64_t(group)
                   + uint32_t(Group_size + Cluster_size * group.d.clusterCount + (((4 * group.d.clusterCount) + 15) & ~15)))
      .d[clusterIndex];
}


// 鍑芥暟锛欳luster_getGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?Group_in Cluster_getGroup(Cluster_in cluster)
Group_in Cluster_getGroup(Cluster_in cluster)
{
  return Group_in(uint64_t(cluster) - uint32_t(cluster.d.groupChildIndex * Cluster_size + Group_size));
}


// 鍑芥暟锛欳luster_getBBox銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?BBox Cluster_getBBox(Cluster_in cluster)
BBox Cluster_getBBox(Cluster_in cluster)
{
  return Group_getClusterBBox(Cluster_getGroup(cluster), cluster.d.groupChildIndex);
}
#endif

#ifdef __cplusplus


// 缁撴瀯锛歂odeRange銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct NodeRange
struct NodeRange
{
  uint32_t isGroup : 1;
  uint32_t childOffset : 26;
  uint32_t childCountMinusOne : 5;
};


// 缁撴瀯锛欸roupRange銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct GroupRange
struct GroupRange
{
  uint32_t isGroup : 1;
  uint32_t groupIndex : 23;
  uint32_t groupClusterCountMinusOne : 8;
};
#endif


// 缁撴瀯锛歂ode銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct Node
struct Node
{
#ifdef __cplusplus
  union
  {
    NodeRange  nodeRange;
    GroupRange groupRange;
  };
#else
  uint32_t packed;

#define Node_packed_isGroup 0 : 1

#define Node_packed_nodeChildOffset 1 : 26
#define Node_packed_nodeChildCountMinusOne 27 : 5

#define Node_packed_groupIndex 1 : 23
#define Node_packed_groupClusterCountMinusOne 24 : 8

#endif

  TraversalMetric traversalMetric;
};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_ARRAY(Nodes_in, Node, readonly, 8);
BUFFER_REF_DECLARE_ARRAY(Nodes_in, Node, readonly, 8);


// 缁撴瀯锛歀odLevel銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct LodLevel
struct LodLevel
{
  float    minBoundingSphereRadius;
  float    minMaxQuadricError;
  uint32_t groupOffset;
  uint32_t groupCount;
  uint32_t clusterOffset;
  uint32_t clusterCount;
};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_ARRAY(LodLevels_inout, LodLevel, , 8);
BUFFER_REF_DECLARE_ARRAY(LodLevels_inout, LodLevel, , 8);


// 缁撴瀯锛欸eometry銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct Geometry
struct Geometry
{
  uint32_t instancesOffset;
  uint32_t instancesCount;
  uint8_t  lodLevelsCount;


  uint16_t lowDetailTriangles;
  uint32_t lowDetailClusterID;


  BBox bbox;

  BUFFER_REF(LodLevels_inout) lodLevels;


  BUFFER_REF(Nodes_in) nodes;
  BUFFER_REF(BBoxes_in) nodeBboxes;


  BUFFER_REF(uint64s_inout) streamingGroupAddresses;


  BUFFER_REF(uint64s_in) preloadedGroups;
  BUFFER_REF(uint64s_in) preloadedClusters;
};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE(Geometry_in, Geometry, readonly, 16);
BUFFER_REF_DECLARE(Geometry_in, Geometry, readonly, 16);


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE(Geometry_inout, Geometry, , 16);
BUFFER_REF_DECLARE(Geometry_inout, Geometry, , 16);


// 缁撴瀯锛歊enderInstance銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct RenderInstance
struct RenderInstance
{
  mat4x3 worldMatrix;
  mat4x3 worldMatrixI;
  uint32_t geometryID;
  uint16_t materialID;
  uint8_t  flipWinding;
  uint8_t  twoSided;
  float    maxLodLevelRcp;
  uint32_t packedColor;
  uint32_t assemblyID;
  float    lodErrorScale;
  uint32_t lodPolicyFlags;
  uint32_t _pad2;
};


// GPU 鎸囬拡澹版槑锛氫负璁惧鍦板潃璁块棶寤虹珛缁撴瀯鍖栫紦鍐插紩鐢ㄧ被鍨嬨€?// 璇ユ満鍒跺厑璁哥潃鑹插櫒閫氳繃 64 浣嶅湴鍧€璁块棶 group銆乧luster銆乶ode 绛夎繍琛屾椂鏁版嵁銆?BUFFER_REF_DECLARE_ARRAY(RenderInstances_in, RenderInstance, readonly, 16);
BUFFER_REF_DECLARE_ARRAY(RenderInstances_in, RenderInstance, readonly, 16);

#ifdef __cplusplus
}
#endif
#endif

//==============================================================================
// 鏂囦欢锛歴haders/interface/shaderio.h
// 妯″潡瀹氫綅锛欳PU 涓?GPU 鍏变韩甯冨眬鏂囦欢锛屽畾涔夌潃鑹插櫒鍜?C++ 鍏卞悓鐞嗚В鐨勬暟鎹粨鏋勩€佸父閲忓拰璁块棶绾﹀畾銆?// 鏁版嵁娴侊細CPU 渚у～鍏呰繖浜涚粨鏋勶紝GPU 渚ф寜瀹屽叏鐩稿悓鐨勫唴瀛樺竷灞€璇诲彇鍜屽啓鍥炪€?// 鏂规硶璇存槑锛氬叡浜竷灞€鏄紓鏋勭郴缁熺殑 ABI锛屼换浣曞瓧娈甸『搴忋€佸榻愬拰浣嶅煙鍙樺寲閮戒細褰卞搷涓や晶瑙ｉ噴涓€鑷存€с€?// 姝ｇ‘鎬х害鏉燂細缁撴瀯瀵归綈銆佹爣閲忓竷灞€鍜?缂撳啿 reference 绫诲瀷蹇呴』涓?Vulkan/GLSL 缂栬瘧閫夐」涓€鑷淬€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?GPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
#ifndef _SHADERIO_H_


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define _SHADERIO_H_
#define _SHADERIO_H_


// 渚濊禆璇存槑锛氬紩鍏ュ叡浜竷灞€銆佸墧闄ゃ€佺潃鑹叉垨闃舵闂村鐢ㄧ殑鐫€鑹插櫒鐗囨銆?// 杩欎簺 include 鍏卞悓鍐冲畾鏈枃浠惰兘璁块棶鐨勭粨鏋勫竷灞€銆佹暟瀛﹁緟鍔╁嚱鏁板拰缂栬瘧鏈熷畯銆?#include "shaderio_core.h"
#include "shaderio_core.h"
#include "shaderio_scene.h"
#include "shaderio_streaming.h"
#include "shaderio_building.h"
#include "nvshaders/sky_io.h.slang"


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_MATERIAL 0
#define VISUALIZE_MATERIAL 0


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_GREY 1
#define VISUALIZE_GREY 1


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_VIS_BUFFER 2
#define VISUALIZE_VIS_BUFFER 2


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_CLUSTER 3
#define VISUALIZE_CLUSTER 3


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_GROUP 4
#define VISUALIZE_GROUP 4


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_LOD 5
#define VISUALIZE_LOD 5


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_TRIANGLE 6
#define VISUALIZE_TRIANGLE 6


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VISUALIZE_DEPTH_ONLY 7
#define VISUALIZE_DEPTH_ONLY 7
#define VISUALIZE_SEMANTIC_LOD 8


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define MESHSHADER_BBOX_VERTICES 8
#define MESHSHADER_BBOX_VERTICES 8


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define MESHSHADER_BBOX_LINES 12
#define MESHSHADER_BBOX_LINES 12


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define MESHSHADER_BBOX_THREADS 4
#define MESHSHADER_BBOX_THREADS 4


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_FRAME_UBO 0
#define BINDINGS_FRAME_UBO 0


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_READBACK_SSBO 1
#define BINDINGS_READBACK_SSBO 1


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_GEOMETRIES_SSBO 2
#define BINDINGS_GEOMETRIES_SSBO 2


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_RENDERINSTANCES_SSBO 3
#define BINDINGS_RENDERINSTANCES_SSBO 3


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_SCENEBUILDING_SSBO 4
#define BINDINGS_SCENEBUILDING_SSBO 4


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_SCENEBUILDING_UBO 5
#define BINDINGS_SCENEBUILDING_UBO 5


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_HIZ_TEX 6
#define BINDINGS_HIZ_TEX 6


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_STREAMING_UBO 7
#define BINDINGS_STREAMING_UBO 7


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_STREAMING_SSBO 8
#define BINDINGS_STREAMING_SSBO 8


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BINDINGS_RENDER_TARGET 10
#define BINDINGS_RENDER_TARGET 10


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BUILD_SETUP_TRAVERSAL_RUN 1
#define BUILD_SETUP_TRAVERSAL_RUN 1


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define BUILD_SETUP_DRAW 2
#define BUILD_SETUP_DRAW 2


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define TRAVERSAL_PRESORT_WORKGROUP 128
#define TRAVERSAL_PRESORT_WORKGROUP 128


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define TRAVERSAL_INIT_WORKGROUP 64
#define TRAVERSAL_INIT_WORKGROUP 64


#define ASSEMBLY_VISIBILITY_WORKGROUP 64


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define TRAVERSAL_RUN_WORKGROUP 64
#define TRAVERSAL_RUN_WORKGROUP 64


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define TRAVERSAL_GROUPS_WORKGROUP 64
#define TRAVERSAL_GROUPS_WORKGROUP 64


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define INSTANCES_CLASSIFY_LOD_WORKGROUP 64
#define INSTANCES_CLASSIFY_LOD_WORKGROUP 64


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define STREAM_UPDATE_SCENE_WORKGROUP 64
#define STREAM_UPDATE_SCENE_WORKGROUP 64


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define STREAM_AGEFILTER_GROUPS_WORKGROUP 96
#define STREAM_AGEFILTER_GROUPS_WORKGROUP 96


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?

// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define FORCE_INVISIBLE_CULLED_REMOVES_INSTANCE 1
#define FORCE_INVISIBLE_CULLED_REMOVES_INSTANCE 1

#ifdef __cplusplus


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace shaderio {
namespace shaderio {
using namespace glm;

#else

#ifndef ALLOW_SHADING


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define ALLOW_SHADING 1
#define ALLOW_SHADING 1
#endif

#ifndef ALLOW_VERTEX_NORMALS


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define ALLOW_VERTEX_NORMALS 1
#define ALLOW_VERTEX_NORMALS 1
#endif

#ifndef ALLOW_VERTEX_TEXCOORDS


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define ALLOW_VERTEX_TEXCOORDS 1
#define ALLOW_VERTEX_TEXCOORDS 1
#endif

#ifndef ALLOW_VERTEX_TEXCOORD_1


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define ALLOW_VERTEX_TEXCOORD_1 1
#define ALLOW_VERTEX_TEXCOORD_1 1
#endif

#ifndef ALLOW_VERTEX_TANGENTS


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define ALLOW_VERTEX_TANGENTS 1
#define ALLOW_VERTEX_TANGENTS 1
#endif

#ifndef USE_RENDER_STATS


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_RENDER_STATS 1
#define USE_RENDER_STATS 1
#endif

#ifndef USE_MEMORY_STATS


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_MEMORY_STATS 1
#define USE_MEMORY_STATS 1
#endif

#ifndef USE_CULLING


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_CULLING 1
#define USE_CULLING 1
#endif

#ifndef USE_FORCED_INVISIBLE_CULLING


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_FORCED_INVISIBLE_CULLING 1
#define USE_FORCED_INVISIBLE_CULLING 1
#endif

#ifndef USE_TWO_PASS_CULLING


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_TWO_PASS_CULLING 1
#define USE_TWO_PASS_CULLING 1
#endif


#ifndef USE_INSTANCE_SORTING


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_INSTANCE_SORTING 1
#define USE_INSTANCE_SORTING 1
#endif


#ifndef USE_STREAMING


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_STREAMING 0
#define USE_STREAMING 0
#endif

#ifndef USE_TWO_SIDED


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_TWO_SIDED 1
#define USE_TWO_SIDED 1
#endif

#ifndef USE_FORCED_TWO_SIDED


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define USE_FORCED_TWO_SIDED 0
#define USE_FORCED_TWO_SIDED 0
#endif

#ifndef MAX_VISIBLE_CLUSTERS


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define MAX_VISIBLE_CLUSTERS 1024
#define MAX_VISIBLE_CLUSTERS 1024
#endif

#ifndef TARGETS_RASTERIZATION


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define TARGETS_RASTERIZATION 1
#define TARGETS_RASTERIZATION 1
#endif


#endif


// 缁撴瀯锛欶rameConstants銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct FrameConstants
struct FrameConstants
{
  mat4 projMatrix;
  mat4 projMatrixI;

  mat4 viewProjMatrix;
  mat4 viewProjMatrixI;
  mat4 viewMatrix;
  mat4 viewMatrixI;
  vec4 viewPos;
  vec4 viewDir;
  vec4 viewPlane;

  mat4 skyProjMatrixI;


  mat4 viewProjMatrixPrev;

  ivec2 viewport;
  vec2  viewportf;

  vec2 viewPixelSize;
  vec2 viewClipSize;

  vec3  wLightPos;
  float lightMixer;

  vec3  wUpDir;
  float sceneSize;


  uint  colorXor;
  uint  visualize;
  float fov;

  float   nearPlane;
  float   farPlane;
  float   ambientOcclusionRadius;
  int32_t ambientOcclusionSamples;

  vec4 hizSizeFactors;
  vec4 nearSizeFactors;

  float hizSizeMax;
  int   facetShading;
  vec2  jitter;

  uint  dbgUint;
  float dbgFloat;
  uint  frame;
  uint  doShadow;

  vec4 bgColor;

  uvec2 mousePosition;
  float wireThickness;
  float wireSmoothing;

  vec3 wireColor;
  uint wireStipple;

  vec3  wireBackfaceColor;
  float wireStippleRepeats;

  float wireStippleLength;
  uint  doWireframe;
  uint  visFilterInstanceID;
  uint  visFilterClusterID;


  float time;
  float deltaTime;
  float lodTransitionSpeed;

  SkySimpleParameters skyParams;
};


// 缁撴瀯锛歊eadback銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct Readback
struct Readback
{
  uint     numRenderClusters;

  uint     numTraversalTasks;

  uint     numTraversedTasks;
  uint     numRenderedClusters;
  uint64_t numRenderedTriangles;
  uint     numRenderedClustersPass0;
  uint     numRenderedClustersPass1;
  uint     numTraversedTasksPass0;
  uint     numTraversedTasksPass1;
  uint     twoPassCullingActive;


#ifdef __cplusplus
  uint32_t clusterTriangleId;
  uint32_t _packedDepth0;

  uint32_t instanceId;
  uint32_t _packedDepth1;
#else
  uint64_t clusterTriangleId;
  uint64_t instanceId;
#endif

  uint64_t debugU64;

  int  debugI;
  uint debugUI;
  uint debugF;

  uint debugA[64];
  uint debugB[64];
  uint debugC[64];
};


#ifdef __cplusplus
}
#endif
#endif

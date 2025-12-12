#include "Registry.h"
#include "EntityObject.h" // ここでEntityObjectの定義を取り込む

// Registryクラスのメンバ関数の実装
EntityObject Registry::CreateEntityObject() {
    Entity id = CreateEntityID();
    // ここではEntityObjectのコンストラクタが見えているのでエラーにならない
    return EntityObject(id, this);
}
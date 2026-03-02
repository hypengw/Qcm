module;
#include "kstore/qt/gadget_model.hpp"
#include "kstore/item_trait.hpp"
#include "kstore/share_store.hpp"
#include "kstore/qt/meta_utils.hpp"
#include "kstore/qt/qtable_proxy_model.hpp"
export module qcm.qt:kstore;

export namespace kstore
{
using kstore::QGadgetListModel;
using kstore::QMetaListModelCRTP;
using kstore::QMetaListModel;
using kstore::QMetaRoleNames;
using kstore::ItemTrait;
using kstore::ShareStore;
using kstore::ListStoreType;
using kstore::QListInterface;
using kstore::QTableProxyModel;
using kstore::qvariant_to_josn;
using kstore::qvariant_from_josn;

}
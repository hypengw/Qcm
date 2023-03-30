import QtQuick
import QcmApp
import ".."

QtObject {
    default property list<QtObject> m_apis: ([])

    id: root

    function status_slot() {
        // 'this' binded to api
        if (this.status === ApiQuerierBase.Error) {
            // ignore 'Operation aborted'
            if(!this.error.endsWith('Operation aborted.'))
                QA.toast(this.error, 5000);
        }

    }

    Component.onCompleted: {
        for (let i = 0; i < m_apis.length; i++) {
            const api = m_apis[i];
            if (api.statusChanged)
                api.statusChanged.connect(status_slot.bind(api));

        }
    }
}

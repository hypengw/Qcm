import QtQuick
import QtTest
import Qcm.App as QA
import "../../../app/qml/action" as Production

TestCase {
    id: test
    name: "CopyAction"
    Component { id: actionComponent; Production.CopyAction {} }
    SignalSpy { id: copied; signalName: "copied" }
    function init() {
        QA.Clipboard.text = "previous clipboard";
        QA.Clipboard.supported = true;
        QA.Clipboard.fail = false;
        QA.Clipboard.writes = 0;
        QA.Action.messages = [];
        copied.target = null;
        copied.clear();
    }
    function action(properties) {
        const value = createTemporaryObject(actionComponent, test, properties || {});
        verify(value !== null);
        copied.target = value;
        return value;
    }
    function test_empty_does_not_clear_clipboard() {
        const value = action();
        verify(!value.enabled);
        value.trigger();
        compare(QA.Clipboard.text, "previous clipboard");
        compare(QA.Clipboard.writes, 0);
        compare(QA.Action.messages.length, 0);
        value.copyText = " \n ";
        verify(!value.enabled);
    }
    function test_success_preserves_text_and_reports_completion() {
        const text = "  \u6d4b\u8bd5 - Artist\nAlbum: Name  ";
        const value = action({copyText: text});
        verify(value.enabled);
        value.trigger();
        compare(QA.Clipboard.text, text);
        compare(QA.Clipboard.writes, 1);
        compare(copied.count, 1);
        compare(QA.Action.messages[0], "Copied to clipboard");
        verify(!value.closeMenu);
    }
    function test_failure_does_not_report_success() {
        const value = action({copyText: "Title"});
        QA.Clipboard.fail = true;
        value.trigger();
        compare(QA.Clipboard.text, "previous clipboard");
        compare(copied.count, 0);
        compare(QA.Action.messages[0], "Clipboard is unavailable");
    }
    function test_disabled_while_busy_or_unsupported() {
        const value = action({copyText: "Title"});
        value.busy = 1;
        verify(!value.enabled);
        value.busy = 0;
        verify(value.enabled);
        QA.Clipboard.supported = false;
        verify(!value.enabled);
    }
    function test_binding_and_legacy_callback() {
        const value = action({getCopyString: () => "error details"});
        value.trigger();
        compare(QA.Clipboard.text, "error details");
        value.getCopyString = () => null;
        verify(!value.enabled);
        value.getCopyString = null;
        value.copyText = "updated title";
        verify(value.enabled);
        value.trigger();
        compare(QA.Clipboard.text, "updated title");
    }
}

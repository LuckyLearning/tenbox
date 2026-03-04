#include "manager/ui/win32_dialogs.h"
#include "manager/ui/dlg_builder.h"
#include "manager/i18n.h"
#include "manager/manager_service.h"

#include <commctrl.h>
#include <string>

enum SfDlgId {
    IDC_SF_LIST    = 300,
    IDC_SF_ADD     = 301,
    IDC_SF_REMOVE  = 302,
};

struct SfDlgData {
    ManagerService* mgr;
    std::string vm_id;
    HWND listview;
};

static void SfRefreshList(SfDlgData* data) {
    HWND lv = data->listview;
    ListView_DeleteAllItems(lv);

    auto folders = data->mgr->GetSharedFolders(data->vm_id);
    for (size_t i = 0; i < folders.size(); ++i) {
        const auto& sf = folders[i];
        LVITEMA item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        item.pszText = const_cast<char*>(sf.tag.c_str());
        int idx = ListView_InsertItem(lv, &item);
        ListView_SetItemText(lv, idx, 1, const_cast<char*>(sf.host_path.c_str()));
        ListView_SetItemText(lv, idx, 2,
            const_cast<char*>(sf.readonly
                ? i18n::tr(i18n::S::kSfModeReadOnly)
                : i18n::tr(i18n::S::kSfModeReadWrite)));
    }
}

static INT_PTR CALLBACK SfDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* data = reinterpret_cast<SfDlgData*>(GetWindowLongPtrA(dlg, DWLP_USER));

    switch (msg) {
    case WM_INITDIALOG: {
        data = reinterpret_cast<SfDlgData*>(lp);
        SetWindowLongPtrA(dlg, DWLP_USER, reinterpret_cast<LONG_PTR>(data));

        RECT rc;
        GetClientRect(dlg, &rc);
        RECT du = {0, 0, 48, 14};
        MapDialogRect(dlg, &du);
        int btn_w = du.right, btn_h = du.bottom;
        int gap = btn_h / 2, btn_gap = btn_h / 4;
        int list_w = rc.right - btn_w - gap * 3;
        int list_h = rc.bottom - gap * 2;

        HWND lv = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            gap, gap, list_w, list_h,
            dlg, reinterpret_cast<HMENU>(IDC_SF_LIST),
            GetModuleHandle(nullptr), nullptr);
        ListView_SetExtendedListViewStyle(lv,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNA col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.cx = 110;
        col.pszText = const_cast<char*>(i18n::tr(i18n::S::kSfColTag));
        ListView_InsertColumn(lv, 0, &col);
        col.cx = list_w - 110 - 90 - 4;
        if (col.cx < 80) col.cx = 80;
        col.pszText = const_cast<char*>(i18n::tr(i18n::S::kSfColHostPath));
        ListView_InsertColumn(lv, 1, &col);
        col.cx = 90;
        col.pszText = const_cast<char*>(i18n::tr(i18n::S::kSfColMode));
        ListView_InsertColumn(lv, 2, &col);

        data->listview = lv;

        int btn_x = gap + list_w + gap;
        MoveWindow(GetDlgItem(dlg, IDC_SF_ADD),    btn_x, gap,                          btn_w, btn_h, FALSE);
        MoveWindow(GetDlgItem(dlg, IDC_SF_REMOVE), btn_x, gap + btn_h + btn_gap,        btn_w, btn_h, FALSE);

        SfRefreshList(data);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_SF_ADD: {
            auto path_str = BrowseForFolder(dlg, i18n::tr(i18n::S::kSfBrowseTitle), nullptr);
            if (!path_str.empty()) {
                size_t last_sep = path_str.find_last_of("\\/");
                std::string tag = (last_sep != std::string::npos)
                    ? path_str.substr(last_sep + 1) : "share";
                if (tag.empty()) tag = "share";

                SharedFolder sf;
                sf.tag = tag;
                sf.host_path = path_str;
                sf.readonly = false;

                std::string error;
                if (data->mgr->AddSharedFolder(data->vm_id, sf, &error)) {
                    SfRefreshList(data);
                } else {
                    MessageBoxA(dlg, error.c_str(),
                        i18n::tr(i18n::S::kError), MB_OK | MB_ICONERROR);
                }
            }
            return TRUE;
        }
        case IDC_SF_REMOVE: {
            int sel = ListView_GetNextItem(data->listview, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBoxA(dlg, i18n::tr(i18n::S::kSfNoSelection),
                    i18n::tr(i18n::S::kError), MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            char tag_buf[64]{};
            ListView_GetItemText(data->listview, sel, 0, tag_buf, sizeof(tag_buf));
            std::string prompt = i18n::fmt(i18n::S::kSfConfirmRemoveMsg, tag_buf);
            if (MessageBoxA(dlg, prompt.c_str(),
                    i18n::tr(i18n::S::kSfConfirmRemoveTitle),
                    MB_YESNO | MB_ICONQUESTION) == IDYES) {
                std::string error;
                if (data->mgr->RemoveSharedFolder(data->vm_id, tag_buf, &error)) {
                    SfRefreshList(data);
                } else {
                    MessageBoxA(dlg, error.c_str(),
                        i18n::tr(i18n::S::kError), MB_OK | MB_ICONERROR);
                }
            }
            return TRUE;
        }
        }
        break;

    case WM_CLOSE:
        EndDialog(dlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

void ShowSharedFoldersDialog(HWND parent, ManagerService& mgr, const std::string& vm_id) {
    using S = i18n::S;
    DlgBuilder b;
    int W = 380, H = 200;
    b.Begin(i18n::tr(S::kDlgSharedFolders), 0, 0, W, H,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_CENTER | DS_SETFONT);

    int btn_h = 14, btn_w = 50;
    b.AddButton(IDC_SF_ADD,    i18n::tr(S::kSfBtnAdd),    0, 0, btn_w, btn_h);
    b.AddButton(IDC_SF_REMOVE, i18n::tr(S::kSfBtnRemove), 0, 0, btn_w, btn_h);

    SfDlgData data{&mgr, vm_id, nullptr};
    DialogBoxIndirectParamA(GetModuleHandle(nullptr), b.Build(), parent,
        SfDlgProc, reinterpret_cast<LPARAM>(&data));
}

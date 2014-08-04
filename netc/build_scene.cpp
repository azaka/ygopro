#include <array>
#include <algorithm>
#include <iostream>

#include <wx/clipbrd.h>

#include "glbase.h"
#include "sungui.h"
#include "image_mgr.h"

#include "card_data.h"
#include "deck_data.h"
#include "build_scene.h"

namespace ygopro
{
    
    BuildScene::BuildScene() {
        glGenBuffers(1, &index_buffer);
        glGenBuffers(1, &deck_buffer);
        glGenBuffers(1, &back_buffer);
        glGenBuffers(1, &misc_buffer);
        glGenBuffers(1, &result_buffer);
        GLCheckError(__FILE__, __LINE__);
        glBindBuffer(GL_ARRAY_BUFFER, back_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * 4, nullptr, GL_DYNAMIC_DRAW);
        GLCheckError(__FILE__, __LINE__);
        glBindBuffer(GL_ARRAY_BUFFER, deck_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * 256 * 16, nullptr, GL_DYNAMIC_DRAW);
        GLCheckError(__FILE__, __LINE__);
        glBindBuffer(GL_ARRAY_BUFFER, result_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * 10 * 16, nullptr, GL_DYNAMIC_DRAW);
        GLCheckError(__FILE__, __LINE__);
        std::vector<unsigned short> index;
        index.resize(256 * 4 * 6);
        for(int i = 0; i < 256 * 4; ++i) {
            index[i * 6] = i * 4;
            index[i * 6 + 1] = i * 4 + 2;
            index[i * 6 + 2] = i * 4 + 1;
            index[i * 6 + 3] = i * 4 + 1;
            index[i * 6 + 4] = i * 4 + 2;
            index[i * 6 + 5] = i * 4 + 3;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 256 * 4 * 6, &index[0], GL_STATIC_DRAW);
        GLCheckError(__FILE__, __LINE__);
        limit[0] = imageMgr.GetTexture("limit0");
        limit[1] = imageMgr.GetTexture("limit1");
        limit[2] = imageMgr.GetTexture("limit2");
        pool[0] = imageMgr.GetTexture("pool_ocg");
        pool[1] = imageMgr.GetTexture("pool_tcg");
        pool[2] = imageMgr.GetTexture("pool_ex");
        hmask = imageMgr.GetTexture("hmask");
    }
    
    BuildScene::~BuildScene() {
        glDeleteBuffers(1, &index_buffer);
        glDeleteBuffers(1, &deck_buffer);
        glDeleteBuffers(1, &back_buffer);
        glDeleteBuffers(1, &misc_buffer);
        glDeleteBuffers(1, &result_buffer);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::Activate() {
        view_regulation = 0;
        prev_hov = std::make_tuple(0, 0, 0);
        file_dialog = std::make_shared<FileDialog>();
        filter_dialog = std::make_shared<FilterDialog>();
        info_panel = std::make_shared<InfoPanel>();
        auto pnl = sgui::SGPanel::Create(nullptr, {10, 5}, {0, 35});
        pnl->SetSize({-20, 35}, {1.0f, 0.0f});
        pnl->eventKeyDown.Bind([this](sgui::SGWidget& sender, sf::Event::KeyEvent evt)->bool {
            KeyDown(evt);
            return true;
        });
        auto rpnl = sgui::SGPanel::Create(nullptr, {0, 0}, {200, 60});
        rpnl->SetPosition({-210, 40}, {1.0f, 0.0f});
        rpnl->eventKeyDown.Bind([this](sgui::SGWidget& sender, sf::Event::KeyEvent evt)->bool {
            KeyDown(evt);
            return true;
        });
        auto icon_label = sgui::SGIconLabel::Create(pnl, {10, 7}, std::wstring(L"\ue08c").append(stringCfg[L"eui_msg_new_deck"]));
        deck_label = icon_label;
        auto menu_deck = sgui::SGButton::Create(pnl, {250, 5}, {70, 25});
        menu_deck->SetText(stringCfg[L"eui_menu_deck"], 0xff000000);
        menu_deck->eventButtonClick.Bind([this](sgui::SGWidget& sender)->bool {
            PopupMenu::Begin(sceneMgr.GetMousePosition(), 100, [this](int id){
                OnMenuDeck(id);
            })
            .AddButton(stringCfg[L"eui_deck_load"])
            .AddButton(stringCfg[L"eui_deck_save"])
            .AddButton(stringCfg[L"eui_deck_saveas"])
            .AddButton(stringCfg[L"eui_deck_loadstr"])
            .AddButton(stringCfg[L"eui_deck_savestr"])
            .End();
            return true;
        });
        auto menu_tool = sgui::SGButton::Create(pnl, {325, 5}, {70, 25});
        menu_tool->SetText(stringCfg[L"eui_menu_tool"], 0xff000000);
        menu_tool->eventButtonClick.Bind([this](sgui::SGWidget& sender)->bool {
            PopupMenu::Begin(sceneMgr.GetMousePosition(), 100, [this](int id){
                OnMenuTool(id);
            })
            .AddButton(stringCfg[L"eui_tool_clear"])
            .AddButton(stringCfg[L"eui_tool_sort"])
            .AddButton(stringCfg[L"eui_tool_shuffle"])
            .AddButton(stringCfg[L"eui_tool_screenshot"])
            .AddButton(stringCfg[L"eui_tool_saveshot"])
            .AddButton(stringCfg[L"eui_tool_browser"])
            .End();
            return true;
        });
        auto menu_list = sgui::SGButton::Create(pnl, {400, 5}, {70, 25});
        menu_list->SetText(stringCfg[L"eui_menu_list"], 0xff000000);
        menu_list->eventButtonClick.Bind([this](sgui::SGWidget& sender)->bool {
            PopupMenu::Begin(sceneMgr.GetMousePosition(), 100, [this](int id){
                OnMenuList(id);
            })
            .AddButton(stringCfg[L"eui_list_forbidden"])
            .AddButton(stringCfg[L"eui_list_limit"])
            .AddButton(stringCfg[L"eui_list_semilimit"])
            .End();
            return true;
        });
        auto menu_search = sgui::SGButton::Create(pnl, {475, 5}, {70, 25});
        menu_search->SetText(stringCfg[L"eui_menu_search"], 0xff000000);
        menu_search->eventButtonClick.Bind([this](sgui::SGWidget& sender)->bool {
            filter_dialog->Show(sceneMgr.GetMousePosition());
            filter_dialog->SetOKCallback([this](const FilterCondition fc, int lmt)->void {
                Search(fc, lmt);
            });
            return true;
        });
        auto limit_reg = sgui::SGComboBox::Create(pnl, {550, 2}, {150, 30});
        auto& lrs = limitRegulationMgr.GetLimitRegulations();
        for(unsigned int i = 0; i < lrs.size(); ++i)
            limit_reg->AddItem(lrs[i].name, 0xff000000);
        limit_reg->SetSelection(0);
        limit_reg->eventSelChange.Bind([this](sgui::SGWidget& sender, int index)->bool {
            ChangeRegulation(index);
            return true;
        });
        auto show_ex = sgui::SGCheckbox::Create(pnl, {710, 7}, {100, 30});
        show_ex->SetText(stringCfg[L"eui_show_exclusive"], 0xff000000);
        show_ex->SetChecked(true);
        show_ex->eventCheckChange.Bind([this](sgui::SGWidget& sender, bool check)->bool {
            ChangeExclusive(check);
            return true;
        });
        label_result = sgui::SGLabel::Create(rpnl, {10, 10}, L"");
        label_page = sgui::SGLabel::Create(rpnl, {30, 33}, L"");
        auto btn1 = sgui::SGButton::Create(rpnl, {10, 35}, {15, 15});
        auto btn2 = sgui::SGButton::Create(rpnl, {170, 35}, {15, 15});
        btn1->SetTextureRect({136, 74, 15, 15}, {136, 90, 15, 15}, {136, 106, 15, 15});
        btn2->SetTextureRect({154, 74, 15, 15}, {154, 90, 15, 15}, {154, 106, 15, 15});
        btn1->eventButtonClick.Bind([this](sgui::SGWidget& sender)->bool {
            ResultPrevPage();
            return true;
        });
        btn2->eventButtonClick.Bind([this](sgui::SGWidget& sender)->bool {
            ResultNextPage();
            return true;
        });
        RefreshAllCard();
    }
    
    void BuildScene::Update() {
        UpdateBackGround();
        UpdateCard();
        UpdateResult();
        UpdateInfo();
    }
    
    void BuildScene::Draw() {
        glViewport(0, 0, scene_size.x, scene_size.y);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        // background
        imageMgr.BindTexture(0);
        glBindBuffer(GL_ARRAY_BUFFER, back_buffer);
        glVertexPointer(2, GL_FLOAT, sizeof(glbase::VertexVCT), 0);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glbase::VertexVCT), (const GLvoid*)glbase::VertexVCT::color_offset);
        glTexCoordPointer(2, GL_FLOAT, sizeof(glbase::VertexVCT), (const GLvoid*)glbase::VertexVCT::tex_offset);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        GLCheckError(__FILE__, __LINE__);
        // cards
        imageMgr.BindTexture(3);
        // result
        glBindBuffer(GL_ARRAY_BUFFER, result_buffer);
        glVertexPointer(2, GL_FLOAT, sizeof(glbase::VertexVCT), 0);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glbase::VertexVCT), (const GLvoid*)glbase::VertexVCT::color_offset);
        glTexCoordPointer(2, GL_FLOAT, sizeof(glbase::VertexVCT), (const GLvoid*)glbase::VertexVCT::tex_offset);
        glDrawElements(GL_TRIANGLES, 10 * 24, GL_UNSIGNED_SHORT, 0);
        GLCheckError(__FILE__, __LINE__);
        // deck
        glBindBuffer(GL_ARRAY_BUFFER, deck_buffer);
        glVertexPointer(2, GL_FLOAT, sizeof(glbase::VertexVCT), 0);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glbase::VertexVCT), (const GLvoid*)glbase::VertexVCT::color_offset);
        glTexCoordPointer(2, GL_FLOAT, sizeof(glbase::VertexVCT), (const GLvoid*)glbase::VertexVCT::tex_offset);
        size_t deck_sz = current_deck.main_deck.size() + current_deck.extra_deck.size() + current_deck.side_deck.size();
        glDrawElements(GL_TRIANGLES, deck_sz * 24, GL_UNSIGNED_SHORT, 0);
        GLCheckError(__FILE__, __LINE__);
        
    }
    
    void BuildScene::SetSceneSize(v2i sz) {
        scene_size = sz;
        float yrate = 1.0f - 40.0f / sz.y;
        card_size = {0.2f * yrate * sz.y / sz.x, 0.29f * yrate};
        icon_size = {0.08f * yrate * sz.y / sz.x, 0.08f * yrate};
        minx = 50.0f / sz.x * 2.0f - 1.0f;
        maxx = 1.0f - 240.0f / sz.x * 2.0f;
        main_y_spacing = 0.3f * yrate;
        offsety[0] = (0.92f + 1.0f) * yrate - 1.0f;
        offsety[1] = (-0.31f + 1.0f) * yrate - 1.0f;
        offsety[2] = (-0.64f + 1.0f) * yrate - 1.0f;
        max_row_count = (maxx - minx - card_size.x) / (card_size.x * 1.1f);
        if(max_row_count < 10)
            max_row_count = 10;
        main_row_count = max_row_count;
        if((int)current_deck.main_deck.size() > main_row_count * 4)
            main_row_count = (current_deck.main_deck.size() - 1) / 4 + 1;
        dx[0] = (maxx - minx - card_size.x) / (main_row_count - 1);
        int rc1 = std::max((int)current_deck.extra_deck.size(), max_row_count);
        dx[1] = (rc1 == 1) ? 0.0f : (maxx - minx - card_size.x) / (rc1 - 1);
        int rc2 = std::max((int)current_deck.side_deck.size(), max_row_count);
        dx[2] = (rc2 == 1) ? 0.0f : (maxx - minx - card_size.x) / (rc2 - 1);
        UpdateAllCard();
        update_result = true;
    }
    
    void BuildScene::MouseMove(sf::Event::MouseMoveEvent evt) {
        std::shared_ptr<DeckCardData> dcd = nullptr;
        auto pre = GetCard(std::get<0>(prev_hov), std::get<1>(prev_hov));
        if(evt.x >= scene_size.x - 210 && evt.x <= scene_size.x - 10 && evt.y >= 110 && evt.y <= scene_size.y - 10) {
            int new_sel = (int)((evt.y - 110.0f) / ((scene_size.y - 120.0f) / 5.0f)) * 2;
            if(new_sel > 8)
                new_sel = 8;
            new_sel += (evt.x >= scene_size.x - 110) ? 1 : 0;
            if(new_sel != current_sel_result) {
                current_sel_result = new_sel;
                update_result = true;
            }
            if(show_info && result_page * 10 + new_sel < search_result.size())
                ShowCardInfo(search_result[result_page * 10 + new_sel]->code);
            std::get<0>(prev_hov) = 4;
            std::get<1>(prev_hov) = new_sel;
        } else {
            auto hov = GetHoverCard((float)evt.x / scene_size.x * 2.0f - 1.0f, 1.0f - (float)evt.y / scene_size.y * 2.0f);
            dcd = GetCard(std::get<0>(hov), std::get<1>(hov));
            prev_hov = hov;
            if(current_sel_result >= 0) {
                current_sel_result = -1;
                update_result = true;
            }
            if(dcd && show_info)
               ShowCardInfo(dcd->data->code);
        }
        if(dcd != pre) {
            if(pre)
                ChangeHL(pre, 0.1f, 0.0f);
            if(dcd)
                ChangeHL(dcd, 0.1f, 0.7f);
        }
    }
    
    void BuildScene::MouseButtonDown(sf::Event::MouseButtonEvent evt) {
        prev_click = prev_hov;
        if(evt.button == sf::Mouse::Left) {
            show_info_begin = true;
            show_info_time = sceneMgr.GetGameTime();
        }
    }
    
    void BuildScene::MouseButtonUp(sf::Event::MouseButtonEvent evt) {
        if(evt.button == sf::Mouse::Left)
            show_info_begin = false;
        if(prev_hov != prev_click)
            return;
        std::get<0>(prev_click) = 0;
        int pos = std::get<0>(prev_hov);
        if(pos > 0 && pos < 4) {
            int index = std::get<1>(prev_hov);
            if(index < 0)
                return;
            auto dcd = GetCard(pos, index);
            if(dcd == nullptr)
                return;
            if(evt.button == sf::Mouse::Left) {
                if(pos == 1) {
                    ChangeHL(current_deck.main_deck[index], 0.1f, 0.0f);
                    current_deck.side_deck.push_back(current_deck.main_deck[index]);
                    current_deck.main_deck.erase(current_deck.main_deck.begin() + index);
                } else if(pos == 2) {
                    ChangeHL(current_deck.extra_deck[index], 0.1f, 0.0f);
                    current_deck.side_deck.push_back(current_deck.extra_deck[index]);
                    current_deck.extra_deck.erase(current_deck.extra_deck.begin() + index);
                } else if(dcd->data->type & 0x802040) {
                    ChangeHL(current_deck.side_deck[index], 0.1f, 0.0f);
                    current_deck.extra_deck.push_back(current_deck.side_deck[index]);
                    current_deck.side_deck.erase(current_deck.side_deck.begin() + index);
                } else {
                    ChangeHL(current_deck.side_deck[index], 0.1f, 0.0f);
                    current_deck.main_deck.push_back(current_deck.side_deck[index]);
                    current_deck.side_deck.erase(current_deck.side_deck.begin() + index);
                }
                RefreshParams();
                RefreshAllIndex();
                UpdateAllCard();
                SetDeckDirty();
                std::get<0>(prev_hov) = 0;
                sf::Event::MouseMoveEvent e;
                e.x = sceneMgr.GetMousePosition().x;
                e.y = sceneMgr.GetMousePosition().y;
                MouseMove(e);
            } else {
                if(update_status == 1)
                    return;
                update_status = 1;
                unsigned int code = dcd->data->code;
                auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
                MoveTo(dcd, 0.1f, ptr->pos + v2f{card_size.x / 2, -card_size.y / 2}, {0.0f, 0.0f});
                ptr->show_limit = false;
                ptr->show_exclusive = false;
                ptr->update_callback = [pos, index, code, this]() {
                    if(current_deck.RemoveCard(pos, index)) {
                        imageMgr.UnloadCardTexture(code);
                        RefreshParams();
                        RefreshAllIndex();
                        UpdateAllCard();
                        SetDeckDirty();
                        std::get<0>(prev_hov) = 0;
                        sf::Event::MouseMoveEvent e;
                        e.x = sceneMgr.GetMousePosition().x;
                        e.y = sceneMgr.GetMousePosition().y;
                        MouseMove(e);
                        update_status = 0;
                        UpdateCard();
                    }
                };
            }
        } else if(pos == 4) {
            int index = std::get<1>(prev_hov);
            if(result_page * 10 + index >= search_result.size())
                return;
            auto data = search_result[result_page * 10 + index];
            std::shared_ptr<DeckCardData> ptr;
            if(evt.button == sf::Mouse::Left)
                ptr = current_deck.InsertCard(1, -1, data->code, true);
            else
                ptr = current_deck.InsertCard(3, -1, data->code, true);
            if(ptr != nullptr) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(data->code);
                exdata->show_exclusive = show_exclusive;
                auto mpos = sceneMgr.GetMousePosition();
                exdata->pos = {(float)mpos.x / scene_size.x * 2.0f - 1.0f, 1.0f - (float)mpos.y / scene_size.y * 2.0f};
                exdata->size = card_size;
                exdata->hl = 0.0f;
                ptr->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
                RefreshParams();
                RefreshAllIndex();
                UpdateAllCard();
                SetDeckDirty();
            }
        }
    }
    
    void BuildScene::KeyDown(sf::Event::KeyEvent evt) {
        switch(evt.code) {
            case sf::Keyboard::Num1:
                if(evt.alt)
                    ViewRegulation(0);
                break;
            case sf::Keyboard::Num2:
                if(evt.alt)
                    ViewRegulation(1);
                break;
            case sf::Keyboard::Num3:
                if(evt.alt)
                    ViewRegulation(2);
                break;
            default:
                break;
        }
    }
    
    void BuildScene::KeyUp(sf::Event::KeyEvent evt) {
        
    }
    
    void BuildScene::ShowCardInfo(unsigned int code) {
        auto mpos = sceneMgr.GetMousePosition();
        int x = (mpos.x >= 490) ? (mpos.x - 480) : (mpos.x + 80);
        int y = mpos.y - 150;
        if(y < 10)
            y = 10;
        if(y > scene_size.y - 310)
            y = scene_size.y - 310;
        info_panel->ShowInfo(code, {x, y});
    }
    
    void BuildScene::ClearDeck() {
        if(current_deck.main_deck.size() + current_deck.extra_deck.size() + current_deck.side_deck.size() == 0)
            return;
        for(auto& dcd : current_deck.main_deck)
            imageMgr.UnloadCardTexture(dcd->data->code);
        for(auto& dcd : current_deck.extra_deck)
            imageMgr.UnloadCardTexture(dcd->data->code);
        for(auto& dcd : current_deck.side_deck)
            imageMgr.UnloadCardTexture(dcd->data->code);
        current_deck.Clear();
        SetDeckDirty();
    }
    
    void BuildScene::SortDeck() {
        current_deck.Sort();
        RefreshAllIndex();
        UpdateAllCard();
    }
    
    void BuildScene::ShuffleDeck() {
        current_deck.Shuffle();
        RefreshAllIndex();
        UpdateAllCard();
    }
    
    void BuildScene::SetDeckDirty() {
        if(!deck_edited) {
            if(current_file.length() == 0)
                deck_label.lock()->SetText(std::wstring(L"\ue08c").append(stringCfg[L"eui_msg_new_deck"]).append(L"*"), 0xff000000);
            else
                deck_label.lock()->SetText(std::wstring(L"\ue08c").append(current_file).append(L"*"), 0xff000000);
            deck_edited = true;
        }
        view_regulation = 0;
    }
    
    void BuildScene::LoadDeckFromFile(const std::wstring& file) {
        DeckData tempdeck;
        if(tempdeck.LoadFromFile(file)) {
            ClearDeck();
            current_deck = tempdeck;
            current_file = file;
            deck_edited = false;
            view_regulation = 0;
            for(auto& dcd : current_deck.main_deck) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
                exdata->show_exclusive = show_exclusive;
                dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
            }
            for(auto& dcd : current_deck.extra_deck) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
                exdata->show_exclusive = show_exclusive;
                dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
            }
            for(auto& dcd : current_deck.side_deck) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
                exdata->show_exclusive = show_exclusive;
                dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
            }
            if(!deck_label.expired())
                deck_label.lock()->SetText(std::wstring(L"\ue08c").append(current_file), 0xff000000);
        }
        RefreshAllCard();
    }
    
    void BuildScene::LoadDeckFromClipboard() {
        DeckData tempdeck;
        wxTextDataObject tdo;
        wxTheClipboard->Open();
        wxTheClipboard->GetData(tdo);
        wxTheClipboard->Close();
        auto deck_string = tdo.GetText().ToStdString();
        if(deck_string.find("ydk://") == 0 && tempdeck.LoadFromString(deck_string.substr(6))) {
            ClearDeck();
            current_deck = tempdeck;
            SetDeckDirty();
            view_regulation = 0;
            for(auto& dcd : current_deck.main_deck) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
                exdata->show_exclusive = show_exclusive;
                dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
            }
            for(auto& dcd : current_deck.extra_deck) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
                exdata->show_exclusive = show_exclusive;
                dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
            }
            for(auto& dcd : current_deck.side_deck) {
                auto exdata = std::make_shared<BuilderCard>();
                exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
                exdata->show_exclusive = show_exclusive;
                dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
            }
        }
        RefreshAllCard();
    }
    
    void BuildScene::SaveDeckToFile(const std::wstring& file) {
        auto deckfile = file;
        if(deckfile.find(L".ydk") != deckfile.length() - 4)
            deckfile.append(L".ydk");
        current_deck.SaveToFile(deckfile);
        current_file = deckfile;
        deck_edited = false;
        if(!deck_label.expired())
            deck_label.lock()->SetText(std::wstring(L"\ue08c").append(current_file), 0xff000000);
    }
    
    void BuildScene::SaveDeckToClipboard() {
        auto str = current_deck.SaveToString();
        wxString deck_string;
        deck_string.append("ydk://").append(str);
        wxTheClipboard->Open();
        wxTheClipboard->SetData(new wxTextDataObject(deck_string));
        wxTheClipboard->Close();
        MessageBox::ShowOK(L"", stringCfg[L"eui_msg_deck_tostr_ok"]);
    }
    
    void BuildScene::OnMenuDeck(int id) {
        switch(id) {
            case 0:
                file_dialog->Show(stringCfg[L"eui_msg_deck_load"], commonCfg[L"deck_path"], L".ydk");
                file_dialog->SetOKCallback([this](const std::wstring& deck)->void {
                    if(deck_edited || deck != current_file)
                        LoadDeckFromFile(deck);
                });
                break;
            case 1:
                if(current_file.length() == 0) {
                    file_dialog->Show(stringCfg[L"eui_msg_deck_save"], commonCfg[L"deck_path"], L".ydk");
                    file_dialog->SetOKCallback([this](const std::wstring& deck)->void {
                        SaveDeckToFile(deck);
                    });
                } else
                    SaveDeckToFile(current_file);
                break;
            case 2:
                file_dialog->Show(stringCfg[L"eui_msg_deck_save"], commonCfg[L"deck_path"], L".ydk");
                file_dialog->SetOKCallback([this](const std::wstring& deck)->void {
                    SaveDeckToFile(deck);
                });
                break;
            case 3:
                LoadDeckFromClipboard();
                break;
            case 4:
                SaveDeckToClipboard();
                break;
            default:
                break;
        }
    }
    
    void BuildScene::OnMenuTool(int id) {
        switch(id) {
            case 0:
                ClearDeck();
                SetDeckDirty();
                break;
            case 1:
                SortDeck();
                break;
            case 2:
                ShuffleDeck();
                break;
            case 3: {
                unsigned char* image_buff = new unsigned char[scene_size.x * scene_size.y * 4];
                unsigned char* rgb_buff = new unsigned char[scene_size.x * scene_size.y * 3];
                glReadPixels(0, 0, scene_size.x, scene_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image_buff);
                for(unsigned int h = 0; h < scene_size.y; ++h) {
                    for(unsigned int w = 0; w < scene_size.x; ++w) {
                        unsigned int ch = scene_size.y - 1 - h;
                        rgb_buff[h * scene_size.x * 3 + w * 3 + 0] = image_buff[ch * scene_size.x * 4 + w * 4 + 0];
                        rgb_buff[h * scene_size.x * 3 + w * 3 + 1] = image_buff[ch * scene_size.x * 4 + w * 4 + 1];
                        rgb_buff[h * scene_size.x * 3 + w * 3 + 2] = image_buff[ch * scene_size.x * 4 + w * 4 + 2];
                    }
                }
                wxImage* img = new wxImage(scene_size.x, scene_size.y, rgb_buff, true);
                if(true) {
                    wxBitmap bmp(*img);
                    wxTheClipboard->Open();
                    wxTheClipboard->SetData(new wxBitmapDataObject(bmp));
                    wxTheClipboard->Close();
                } else
                    img->SaveFile(L"a.png", wxBITMAP_TYPE_PNG);
                delete img;
                delete[] rgb_buff;
                delete[] image_buff;
            }
                break;
            case 4:
                break;
            case 5: {
                wxString neturl = static_cast<const std::wstring&>(commonCfg[L"deck_neturl"]);
                wxString deck_string = current_deck.SaveToString();
                neturl.Replace("{amp}", wxT("&"));
                neturl.Replace("{deck}", deck_string);
                auto pos = current_file.find_last_of(L'/');
                if(pos == std::wstring::npos)
                    neturl.Replace("{name}", wxEmptyString);
                else
                    neturl.Replace("{name}", current_file.substr(pos + 1));
                wxLaunchDefaultBrowser(neturl);
                break;
            }
            default:
                break;
        }
    }
    
    void BuildScene::OnMenuList(int id) {
        switch(id) {
            case 0:
                ViewRegulation(0);
                break;
            case 1:
                ViewRegulation(1);
                break;
            case 2:
                ViewRegulation(2);
                break;
            default:
                break;
        }
    }
    
    void BuildScene::UpdateBackGround() {
        if(!update_bg)
            return;
        update_bg = false;
        auto ti = imageMgr.GetTexture("bg");
        std::array<glbase::VertexVCT, 4> verts;
        verts[0].vertex = {-1.0f, 1.0f, 0.0f};
        verts[0].texcoord = ti.vert[0];
        verts[1].vertex = {1.0f, 1.0f, 0.0f};
        verts[1].texcoord = ti.vert[1];
        verts[2].vertex = {-1.0f, -1.0f, 0.0f};
        verts[2].texcoord = ti.vert[2];
        verts[3].vertex = {1.0f, -1.0f, 0.0f};
        verts[3].texcoord = ti.vert[3];
        glBindBuffer(GL_ARRAY_BUFFER, back_buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glbase::VertexVCT) * verts.size(), &verts[0]);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::UpdateCard() {
        std::function<void()> f = nullptr;
        float tm = sceneMgr.GetGameTime();
        glBindBuffer(GL_ARRAY_BUFFER, deck_buffer);
        for(auto iter = updating_cards.begin(); iter != updating_cards.end();) {
            auto cur = iter++;
            auto dcd = (*cur);
            if(dcd == nullptr) {
                updating_cards.erase(cur);
                continue;
            }
            auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
            bool up = ptr->update_pos;
            if(ptr->update_pos) {
                if(tm >= ptr->moving_time_e) {
                    ptr->pos = ptr->dest_pos;
                    ptr->size = ptr->dest_size;
                    ptr->update_pos = false;
                } else {
                    float rate = (tm - ptr->moving_time_b) / (ptr->moving_time_e - ptr->moving_time_b);
                    ptr->pos = ptr->start_pos + (ptr->dest_pos - ptr->start_pos) * rate;
                    ptr->size = ptr->start_size + (ptr->dest_size - ptr->start_size) * rate;
                }
            }
            if(ptr->update_color) {
                if(tm >= ptr->fading_time_e)
                {
                    ptr->hl = ptr->dest_hl;
                    ptr->update_color = false;
                } else {
                    float rate = (tm - ptr->fading_time_b) / (ptr->fading_time_e - ptr->fading_time_b);
                    ptr->hl = ptr->start_hl + (ptr->dest_hl - ptr->start_hl) * rate;
                }
            }
            if(up)
                RefreshCardPos(dcd);
            else
                RefreshHL(dcd);
            if(!ptr->update_pos && !ptr->update_color) {
                updating_cards.erase(cur);
                if(ptr->update_callback != nullptr)
                    f = ptr->update_callback;
            }
        }
        if(f != nullptr)
            f();
    }
    
    void BuildScene::UpdateAllCard() {
        update_card = false;
        size_t deck_sz = current_deck.main_deck.size() + current_deck.extra_deck.size() + current_deck.side_deck.size();
        if(deck_sz == 0)
            return;
        for(size_t i = 0; i < current_deck.main_deck.size(); ++i) {
            auto cpos = (v2f){minx + dx[0] * (i % main_row_count), offsety[0] - main_y_spacing * (i / main_row_count)};
            MoveTo(current_deck.main_deck[i], 0.1f, cpos, card_size);
        }
        if(current_deck.extra_deck.size()) {
            for(size_t i = 0; i < current_deck.extra_deck.size(); ++i) {
                auto cpos = (v2f){minx + dx[1] * i, offsety[1]};
                MoveTo(current_deck.extra_deck[i], 0.1f, cpos, card_size);
            }
        }
        if(current_deck.side_deck.size()) {
            for(size_t i = 0; i < current_deck.side_deck.size(); ++i) {
                auto cpos = (v2f){minx + dx[2] * i, offsety[2]};
                MoveTo(current_deck.side_deck[i], 0.1f, cpos, card_size);
            }
        }
    }
    
    void BuildScene::RefreshParams() {
        main_row_count = max_row_count;
        if((int)current_deck.main_deck.size() > main_row_count * 4)
            main_row_count = (current_deck.main_deck.size() - 1) / 4 + 1;
        dx[0] = (maxx - minx - card_size.x) / (main_row_count - 1);
        int rc1 = std::max((int)current_deck.extra_deck.size(), max_row_count);
        dx[1] = (rc1 == 1) ? 0.0f : (maxx - minx - card_size.x) / (rc1 - 1);
        int rc2 = std::max((int)current_deck.side_deck.size(), max_row_count);
        dx[2] = (rc2 == 1) ? 0.0f : (maxx - minx - card_size.x) / (rc2 - 1);
    }
    
    void BuildScene::RefreshAllCard() {
        size_t deck_sz = current_deck.main_deck.size() + current_deck.extra_deck.size() + current_deck.side_deck.size();
        if(deck_sz == 0)
            return;
        RefreshParams();
        glBindBuffer(GL_ARRAY_BUFFER, deck_buffer);
        unsigned int index = 0;
        for(size_t i = 0; i < current_deck.main_deck.size(); ++i) {
            auto cpos = (v2f){minx + dx[0] * (i % main_row_count), offsety[0] - main_y_spacing * (i / main_row_count)};
            auto ptr = std::static_pointer_cast<BuilderCard>(current_deck.main_deck[i]->extra);
            ptr->buffer_index = index++;
            ptr->pos = cpos;
            ptr->size = card_size;
            RefreshCardPos(current_deck.main_deck[i]);
        }
        for(size_t i = 0; i < current_deck.extra_deck.size(); ++i) {
            auto cpos = (v2f){minx + dx[1] * i, offsety[1]};
            auto ptr = std::static_pointer_cast<BuilderCard>(current_deck.extra_deck[i]->extra);
            ptr->buffer_index = index++;
            ptr->pos = cpos;
            ptr->size = card_size;
            RefreshCardPos(current_deck.extra_deck[i]);
        }
        for(size_t i = 0; i < current_deck.side_deck.size(); ++i) {
            auto cpos = (v2f){minx + dx[2] * i, offsety[2]};
            auto ptr = std::static_pointer_cast<BuilderCard>(current_deck.side_deck[i]->extra);
            ptr->buffer_index = index++;
            ptr->pos = cpos;
            ptr->size = card_size;
            RefreshCardPos(current_deck.side_deck[i]);
        }
    }
    
    void BuildScene::RefreshAllIndex() {
        unsigned int index = 0;
        for(size_t i = 0; i < current_deck.main_deck.size(); ++i) {
            auto ptr = std::static_pointer_cast<BuilderCard>(current_deck.main_deck[i]->extra);
            ptr->buffer_index = index++;
        }
        for(size_t i = 0; i < current_deck.extra_deck.size(); ++i) {
            auto ptr = std::static_pointer_cast<BuilderCard>(current_deck.extra_deck[i]->extra);
            ptr->buffer_index = index++;
        }
        for(size_t i = 0; i < current_deck.side_deck.size(); ++i) {
            auto ptr = std::static_pointer_cast<BuilderCard>(current_deck.side_deck[i]->extra);
            ptr->buffer_index = index++;
        }
    }
    
    void BuildScene::RefreshCardPos(std::shared_ptr<DeckCardData> dcd) {
        auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
        std::array<glbase::VertexVCT, 16> verts;
        verts[0].vertex = ptr->pos;
        verts[0].texcoord = ptr->card_tex.ti.vert[0];
        verts[1].vertex = {ptr->pos.x + ptr->size.x, ptr->pos.y, 0.0f};
        verts[1].texcoord = ptr->card_tex.ti.vert[1];
        verts[2].vertex = {ptr->pos.x, ptr->pos.y - ptr->size.y, 0.0f};
        verts[2].texcoord = ptr->card_tex.ti.vert[2];
        verts[3].vertex = {ptr->pos.x + ptr->size.x, ptr->pos.y - ptr->size.y, 0.0f};
        verts[3].texcoord = ptr->card_tex.ti.vert[3];
        unsigned int cl = (((unsigned int)(ptr->hl * 255) & 0xff) << 24) | 0xffffff;
        verts[4].vertex = ptr->pos;
        verts[4].texcoord = hmask.vert[0];
        verts[4].color = cl;
        verts[5].vertex = {ptr->pos.x + ptr->size.x, ptr->pos.y, 0.0f};
        verts[5].texcoord = hmask.vert[1];
        verts[5].color = cl;
        verts[6].vertex = {ptr->pos.x, ptr->pos.y - ptr->size.y, 0.0f};
        verts[6].texcoord = hmask.vert[2];
        verts[6].color = cl;
        verts[7].vertex = {ptr->pos.x + ptr->size.x, ptr->pos.y - ptr->size.y, 0.0f};
        verts[7].texcoord = hmask.vert[3];
        verts[7].color = cl;
        if(dcd->limit < 3) {
            auto& lti = limit[dcd->limit];
            verts[8].vertex = {ptr->pos.x - 0.01f, ptr->pos.y + 0.01f, 0.0f};
            verts[8].texcoord = lti.vert[0];
            verts[9].vertex = {ptr->pos.x - 0.01f + icon_size.x, ptr->pos.y + 0.01f, 0.0f};
            verts[9].texcoord = lti.vert[1];
            verts[10].vertex = {ptr->pos.x - 0.01f, ptr->pos.y + 0.01f - icon_size.y, 0.0f};
            verts[10].texcoord = lti.vert[2];
            verts[11].vertex = {ptr->pos.x - 0.01f + icon_size.x, ptr->pos.y + 0.01f - icon_size.y, 0.0f};
            verts[11].texcoord = lti.vert[3];
        }
        if((ptr->show_exclusive) && dcd->data->pool != 3) {
            float px = ptr->pos.x + ptr->size.x / 2.0f - icon_size.x * 0.75f;
            auto& pti = (dcd->data->pool == 1) ? pool[0] : pool[1];
            verts[12].vertex = {px, ptr->pos.y - ptr->size.y + icon_size.y * 0.75f - 0.01f, 0.0f};
            verts[12].texcoord = pti.vert[0];
            verts[13].vertex = {px + icon_size.x * 1.5f, ptr->pos.y - ptr->size.y + icon_size.y * 0.75f - 0.01f, 0.0f};
            verts[13].texcoord = pti.vert[1];
            verts[14].vertex = {px, ptr->pos.y - ptr->size.y - 0.01f, 0.0f};
            verts[14].texcoord = pti.vert[2];
            verts[15].vertex = {px + icon_size.x * 1.5f, ptr->pos.y - ptr->size.y - 0.01f, 0.0f};
            verts[15].texcoord = pti.vert[3];
        }
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * ptr->buffer_index * 16, sizeof(glbase::VertexVCT) * 16, &verts[0]);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::UpdateResult() {
        if(!update_result)
            return;
        update_result = false;
        if(result_page * 10 >= search_result.size())
            return;
        float left = 1.0f - 210.0f / scene_size.x * 2.0f;
        float right = 1.0f - 10.0f / scene_size.x * 2.0f;
        float width = (right - left) / 2.0f;
        float top = 1.0f - 110.0f / scene_size.y * 2.0f;
        float down = 10.0f / scene_size.y * 2.0f - 1.0f;
        float height = (top - down) / 5.0f;
        float cheight = height * 0.9f;
        float cwidth = cheight / 2.9f * 2.0f * scene_size.y / scene_size.x;
        float offy = height * 0.05f;
        float iheight = 0.08f / 0.29f * cheight;
        float iwidth = iheight * scene_size.y / scene_size.x;
        
        std::array<glbase::VertexVCT, 160> verts;
        for(int i = 0; i < 10 ; ++i) {
            if(i + result_page * 10 >= search_result.size())
                continue;
            auto pvert = &verts[i * 16];
            unsigned int cl = (i == current_sel_result) ? 0xc0ffffff : 0xc0000000;
            pvert[0].vertex = {left + (i % 2) * width, top - (i / 2) * height, 0.0f};
            pvert[0].texcoord = hmask.vert[0];
            pvert[0].color = cl;
            pvert[1].vertex = {left + (i % 2) * width + width, top - (i / 2) * height, 0.0f};
            pvert[1].texcoord = hmask.vert[1];
            pvert[1].color = cl;
            pvert[2].vertex = {left + (i % 2) * width, top - (i / 2) * height - height, 0.0f};
            pvert[2].texcoord = hmask.vert[2];
            pvert[2].color = cl;
            pvert[3].vertex = {left + (i % 2) * width + width, top - (i / 2) * height - height, 0.0f};
            pvert[3].texcoord = hmask.vert[3];
            pvert[3].color = cl;
            CardData* pdata = search_result[i + result_page * 10];
            pvert[4].vertex = {left + (i % 2) * width + width / 2 - cwidth / 2, top - (i / 2) * height - offy, 0.0f};
            pvert[4].texcoord = result_tex[i].ti.vert[0];
            pvert[5].vertex = {left + (i % 2) * width + width / 2 + cwidth / 2, top - (i / 2) * height - offy, 0.0f};
            pvert[5].texcoord = result_tex[i].ti.vert[1];
            pvert[6].vertex = {left + (i % 2) * width + width / 2 - cwidth / 2, top - (i / 2) * height - height + offy, 0.0f};
            pvert[6].texcoord = result_tex[i].ti.vert[2];
            pvert[7].vertex = {left + (i % 2) * width + width / 2 + cwidth / 2, top - (i / 2) * height - height + offy, 0.0f};
            pvert[7].texcoord = result_tex[i].ti.vert[3];
            unsigned int lmt = limitRegulationMgr.GetCardLimitCount(pdata->code);
            if(lmt < 3) {
                pvert[8].vertex = {left + (i % 2) * width + width / 2 - cwidth / 2 - 0.01f, top - (i / 2) * height - offy + 0.01f, 0.0f};
                pvert[8].texcoord = limit[lmt].vert[0];
                pvert[9].vertex = {left + (i % 2) * width + width / 2 - cwidth / 2 - 0.01f + iwidth, top - (i / 2) * height - offy + 0.01f, 0.0f};
                pvert[9].texcoord = limit[lmt].vert[1];
                pvert[10].vertex = {left + (i % 2) * width + width / 2 - cwidth / 2 - 0.01f, top - (i / 2) * height - offy + 0.01f - iheight, 0.0f};
                pvert[10].texcoord = limit[lmt].vert[2];
                pvert[11].vertex = {left + (i % 2) * width + width / 2 - cwidth / 2 - 0.01f + iwidth, top - (i / 2) * height - offy + 0.01f - iheight, 0.0f};
                pvert[11].texcoord = limit[lmt].vert[3];
            }
            if(show_exclusive && pdata->pool != 3) {
                auto& pti = (pdata->pool == 1) ? pool[0] : pool[1];
                pvert[12].vertex = {left + (i % 2) * width + width / 2 - iwidth * 0.75f, top - (i / 2) * height + offy - height + iheight * 0.75f - 0.01f, 0.0f};
                pvert[12].texcoord = pti.vert[0];
                pvert[13].vertex = {left + (i % 2) * width + width / 2 + iwidth * 0.75f, top - (i / 2) * height + offy - height + iheight * 0.75f - 0.01f, 0.0f};
                pvert[13].texcoord = pti.vert[1];
                pvert[14].vertex = {left + (i % 2) * width + width / 2 - iwidth * 0.75f, top - (i / 2) * height + offy - height - 0.01f, 0.0f};
                pvert[14].texcoord = pti.vert[2];
                pvert[15].vertex = {left + (i % 2) * width + width / 2 + iwidth * 0.75f, top - (i / 2) * height + offy - height - 0.01f, 0.0f};
                pvert[15].texcoord = pti.vert[3];
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, result_buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glbase::VertexVCT) * 160, &verts[0]);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::UpdateInfo() {
        if(show_info_begin) {
            float now = sceneMgr.GetGameTime();
            if(now - show_info_time >= 1.0f) {
                show_info = true;
                show_info_begin = false;
                std::get<0>(prev_click) = 0;
                auto pos = std::get<0>(prev_hov);
                if(pos > 0 && pos < 4) {
                    auto dcd = GetCard(pos, std::get<1>(prev_hov));
                    if(dcd != nullptr)
                        ShowCardInfo(dcd->data->code);
                } else {
                    auto index = std::get<1>(prev_hov);
                    if(result_page * 10 + index < search_result.size())
                        ShowCardInfo(search_result[result_page * 10 + index]->code);
                }
                sgui::SGGUIRoot::GetSingleton().eventMouseButtonUp.Bind([this](sgui::SGWidget& sender, sf::Event::MouseButtonEvent evt)->bool {
                    if(evt.button == sf::Mouse::Left) {
                        sgui::SGGUIRoot::GetSingleton().eventMouseButtonUp.Reset();
                        show_info = false;
                        show_info_begin = false;
                        info_panel->Destroy();
                    }
                    return false;
                });
            }
        }
    }
    
    void BuildScene::RefreshHL(std::shared_ptr<DeckCardData> dcd) {
        auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
        std::array<glbase::VertexVCT, 4> verts;
        unsigned int cl = (((unsigned int)(ptr->hl * 255) & 0xff) << 24) | 0xffffff;
        verts[0].vertex = ptr->pos;
        verts[0].texcoord = hmask.vert[0];
        verts[0].color = cl;
        verts[1].vertex = {ptr->pos.x + ptr->size.x, ptr->pos.y, 0.0f};
        verts[1].texcoord = hmask.vert[1];
        verts[1].color = cl;
        verts[2].vertex = {ptr->pos.x, ptr->pos.y - ptr->size.y, 0.0f};
        verts[2].texcoord = hmask.vert[2];
        verts[2].color = cl;
        verts[3].vertex = {ptr->pos.x + ptr->size.x, ptr->pos.y - ptr->size.y, 0.0f};
        verts[3].texcoord = hmask.vert[3];
        verts[3].color = cl;
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * (ptr->buffer_index * 16 + 4), sizeof(glbase::VertexVCT) * 4, &verts[0]);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::RefreshLimit(std::shared_ptr<DeckCardData> dcd) {
        auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
        std::array<glbase::VertexVCT, 4> verts;
        if(dcd->limit < 3) {
            auto lti = limit[dcd->limit];
            verts[0].vertex = {ptr->pos.x - 0.01f, ptr->pos.y + 0.01f, 0.0f};
            verts[0].texcoord = lti.vert[0];
            verts[1].vertex = {ptr->pos.x - 0.01f + icon_size.x, ptr->pos.y + 0.01f, 0.0f};
            verts[1].texcoord = lti.vert[1];
            verts[2].vertex = {ptr->pos.x - 0.01f, ptr->pos.y + 0.01f - icon_size.y, 0.0f};
            verts[2].texcoord = lti.vert[2];
            verts[3].vertex = {ptr->pos.x - 0.01f + icon_size.x, ptr->pos.y + 0.01f - icon_size.y, 0.0f};
            verts[3].texcoord = lti.vert[3];
        }
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * (ptr->buffer_index * 16 + 8), sizeof(glbase::VertexVCT) * 4, &verts[0]);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::RefreshEx(std::shared_ptr<DeckCardData> dcd) {
        auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
        std::array<glbase::VertexVCT, 4> verts;
        if((ptr->show_exclusive) && dcd->data->pool != 3) {
            float px = ptr->pos.x + ptr->size.x / 2.0f - icon_size.x * 0.75f;
            auto& pti = (dcd->data->pool == 1) ? pool[0] : pool[1];
            verts[0].vertex = {px, ptr->pos.y - ptr->size.y + icon_size.y * 0.75f - 0.01f, 0.0f};
            verts[0].texcoord = pti.vert[0];
            verts[1].vertex = {px + icon_size.x * 1.5f, ptr->pos.y - ptr->size.y + icon_size.y * 0.75f - 0.01f, 0.0f};
            verts[1].texcoord = pti.vert[1];
            verts[2].vertex = {px, ptr->pos.y - ptr->size.y - 0.01f, 0.0f};
            verts[2].texcoord = pti.vert[2];
            verts[3].vertex = {px + icon_size.x * 1.5f, ptr->pos.y - ptr->size.y - 0.01f, 0.0f};
            verts[3].texcoord = pti.vert[3];
        }
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(glbase::VertexVCT) * (ptr->buffer_index * 16 + 12), sizeof(glbase::VertexVCT) * 4, &verts[0]);
        GLCheckError(__FILE__, __LINE__);
    }
    
    void BuildScene::MoveTo(std::shared_ptr<DeckCardData> dcd, float tm, v2f dst, v2f dsz) {
        auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
        ptr->start_pos = ptr->pos;
        ptr->start_size = ptr->size;
        ptr->dest_pos = dst;
        ptr->dest_size = dsz;
        ptr->moving_time_b = sceneMgr.GetGameTime();
        ptr->moving_time_e = ptr->moving_time_b + tm;
        ptr->update_pos = true;
        updating_cards.insert(dcd);
    }
    
    void BuildScene::ChangeHL(std::shared_ptr<DeckCardData> dcd, float tm, float desthl) {
        auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
        ptr->start_hl = ptr->hl;
        ptr->dest_hl = desthl;
        ptr->fading_time_b = sceneMgr.GetGameTime();
        ptr->fading_time_e = ptr->fading_time_b + tm;
        ptr->update_color = true;
        updating_cards.insert(dcd);
    }
    
    void BuildScene::ChangeExclusive(bool check) {
        show_exclusive = check;
        for(auto& dcd : current_deck.main_deck) {
            auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
            ptr->show_exclusive = show_exclusive;
            RefreshEx(dcd);
        }
        for(auto& dcd : current_deck.extra_deck) {
            auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
            ptr->show_exclusive = show_exclusive;
            RefreshEx(dcd);
        }
        for(auto& dcd : current_deck.side_deck) {
            auto ptr = std::static_pointer_cast<BuilderCard>(dcd->extra);
            ptr->show_exclusive = show_exclusive;
            RefreshEx(dcd);
        }
        update_result = true;
    }
    
    void BuildScene::ChangeRegulation(int index) {
        limitRegulationMgr.SetLimitRegulation(index);
        if(view_regulation)
            ViewRegulation(view_regulation - 1);
        else
            limitRegulationMgr.GetDeckCardLimitCount(current_deck);
        RefreshAllCard();
        update_result = true;
    }
    
    void BuildScene::ViewRegulation(int limit) {
        ClearDeck();
        limitRegulationMgr.LoadCurrentListToDeck(current_deck, limit);
        for(auto& dcd : current_deck.main_deck) {
            auto exdata = std::make_shared<BuilderCard>();
            exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
            exdata->show_exclusive = show_exclusive;
            dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
        }
        for(auto& dcd : current_deck.extra_deck) {
            auto exdata = std::make_shared<BuilderCard>();
            exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
            exdata->show_exclusive = show_exclusive;
            dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
        }
        for(auto& dcd : current_deck.side_deck) {
            auto exdata = std::make_shared<BuilderCard>();
            exdata->card_tex = imageMgr.GetCardTexture(dcd->data->code);
            exdata->show_exclusive = show_exclusive;
            dcd->extra = std::static_pointer_cast<DeckCardExtraData>(exdata);
        }
        std::wstring title = L"\ue07a";
        if(limit == 0)
            title.append(stringCfg[L"eui_list_forbidden"]);
        else if(limit == 1)
            title.append(stringCfg[L"eui_list_limit"]);
        else
            title.append(stringCfg[L"eui_list_semilimit"]);
        deck_label.lock()->SetText(title, 0xff000000);
        view_regulation = limit + 1;
        current_file.clear();
        deck_edited = false;
        RefreshAllCard();
    }
    
    void BuildScene::UnloadSearchResult() {
        for(int i = 0; i < 10; ++i) {
            if(i + result_page * 10 >= search_result.size())
                break;
            imageMgr.UnloadCardTexture(search_result[i + result_page * 10]->code);
        }
    }
    
    void BuildScene::RefreshSearchResult() {
        for(int i = 0; i < 10; ++i) {
            if(i + result_page * 10 >= search_result.size())
                break;
            result_tex[i] = imageMgr.GetCardTexture(search_result[i + result_page * 10]->code);
        }
        update_result = true;
        auto ptr = label_page.lock();
        if(ptr != nullptr) {
            int pageall = (search_result.size() == 0) ? 0 : (search_result.size() - 1) / 10 + 1;
            wxString s = wxString::Format(L"%d/%d", result_page + 1, pageall);
            ptr->SetText(s.ToStdWstring(), 0xff000000);
            ptr->SetPosition({100 - ptr->GetSize().x / 2, 33});
        }
    }
    
    void BuildScene::ResultPrevPage() {
        if(result_page == 0)
            return;
        UnloadSearchResult();
        result_page--;
        RefreshSearchResult();
    }
    
    void BuildScene::ResultNextPage() {
        if(result_page * 10 + 10 >= search_result.size())
            return;
        UnloadSearchResult();
        result_page++;
        RefreshSearchResult();
    }
    
    void BuildScene::QuickSearch(const std::wstring& keystr) {
        FilterCondition fc;
        if(keystr.length() > 0) {
            if(keystr[0] == L'@') {
                fc.code = FilterDialog::ParseInt(&keystr[1], keystr.length() - 1);
            } else if(keystr[0] == L'#') {
                std::wstring setstr = L"setname_";
                setstr.append(keystr.substr(1));
                if(stringCfg.Exists(setstr))
                    fc.setcode = stringCfg[setstr];
                else
                    fc.setcode = 0xffff;
            } else
                fc.keyword = keystr;
            UnloadSearchResult();
            search_result = dataMgr.FilterCard(fc);
            std::sort(search_result.begin(), search_result.end(), CardData::card_sort);
            result_page = 0;
            RefreshSearchResult();
        }
    }
    
    void BuildScene::Search(const FilterCondition& fc, int lmt) {
        UnloadSearchResult();
        if(lmt == 0)
            search_result = dataMgr.FilterCard(fc);
        else
            search_result = limitRegulationMgr.FilterCard(lmt - 1, fc);
        std::sort(search_result.begin(), search_result.end(), CardData::card_sort);
        result_page = 0;
        RefreshSearchResult();
    }
    
    std::shared_ptr<DeckCardData> BuildScene::GetCard(int pos, int index) {
        if(index < 0)
            return nullptr;
        if(pos == 1) {
            if(index >= (int)current_deck.main_deck.size())
                return nullptr;
            return current_deck.main_deck[index];
        } else if(pos == 2) {
            if(index >= (int)current_deck.extra_deck.size())
                return nullptr;
            return current_deck.extra_deck[index];
        } else if(pos == 3) {
            if(index >= (int)current_deck.side_deck.size())
                return nullptr;
            return current_deck.side_deck[index];
        }
        return nullptr;
    }
    
    std::tuple<int, int, int> BuildScene::GetHoverCard(float x, float y) {
        if(x >= minx && x <= maxx) {
            if(y <= offsety[0] && y >= offsety[0] - main_y_spacing * 4) {
                unsigned int row = (unsigned int)((offsety[0] - y) / main_y_spacing);
                if(row > 3)
                    row = 3;
                int index = 0;
                if(x > maxx - card_size.x)
                    index = main_row_count - 1;
                else
                    index = (int)((x - minx) / dx[0]);
                int cindex = index;
                index += row * main_row_count;
                if(index >= (int)current_deck.main_deck.size())
                    return std::make_tuple(1, -1, current_deck.side_deck.size());
                else {
                    if(y < offsety[0] - main_y_spacing * row - card_size.y || x > minx + cindex * dx[0] + card_size.x)
                        return std::make_tuple(1, -1, index);
                    else
                        return std::make_tuple(1, index, 0);
                }
            } else if(y <= offsety[1] && y >= offsety[1] - card_size.y) {
                int rc = std::max((int)current_deck.extra_deck.size(), max_row_count);
                int index = 0;
                if(x > maxx - card_size.x)
                    index = rc - 1;
                else
                    index = (int)((x - minx) / dx[1]);
                if(index >= (int)current_deck.extra_deck.size())
                    return std::make_tuple(2, -1, current_deck.extra_deck.size());
                else {
                    if(x > minx + index * dx[1] + card_size.x)
                        return std::make_tuple(2, -1, index);
                    else
                        return std::make_tuple(2, index, 0);
                }
            } else if(y <= offsety[2] && y >= offsety[2] - card_size.y) {
                int rc = std::max((int)current_deck.side_deck.size(), max_row_count);
                int index = 0;
                if(x > maxx - card_size.x)
                    index = rc - 1;
                else
                    index = (int)((x - minx) / dx[2]);
                if(index >= (int)current_deck.side_deck.size())
                    return std::make_tuple(3, -1, current_deck.side_deck.size());
                else {
                    if(x > minx + index * dx[2] + card_size.x)
                        return std::make_tuple(3, -1, index);
                    else
                        return std::make_tuple(3, index, 0);
                }
            }
        }
        return std::make_tuple(0, 0, 0);
    }
    
}
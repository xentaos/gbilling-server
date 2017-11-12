/*
 *  gBilling is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  gBilling is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gBilling; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  callback.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include <gtk/gtk.h>

gint column_sort_func (GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
/* window_init */
void on_window_init_destroy (GtkWidget*, gpointer);

/* window_main */
gboolean on_window_main_delete_event (GtkWidget*, GdkEvent*, gpointer);
#ifdef _WIN32
gboolean on_window_main_state_event (GtkWidget*, GdkEventWindowState*, gpointer);
#endif
void on_selection_client_changed (GtkTreeSelection*, gpointer);
void on_iconv_status_selection_changed (GtkIconView*, gpointer);
void on_selection_log_changed (GtkTreeSelection*, gpointer);
void on_noteb_control_switch_page (GtkNotebook*, GtkNotebookPage*, gint, gpointer);
void on_menu_setting_server_activated (GtkMenuItem*, gpointer);
void on_menu_setting_log_activated (GtkMenuItem*, gpointer data);
void on_menu_setting_mute_toggled (GtkCheckMenuItem*, gpointer);
void on_menu_setting_client_activated (GtkWidget*, gpointer);
void on_menu_setting_cost_activate (GtkWidget*, gpointer);
void on_menu_setting_prepaid_activate (GtkWidget*, gpointer);
void on_menu_setting_member_group_activate (GtkMenuItem*, gpointer);
void on_menu_setting_member_manage_activate (GtkMenuItem*, gpointer);
void on_menu_help_about_activate (GtkWidget*, gpointer);
void on_menu_help_debug_activate (GtkMenuItem*, gpointer);
void on_toggleb_client_toggled (GtkToggleButton*, gpointer);
void on_toggleb_status_toggled (GtkToggleButton*, gpointer);
void on_toggleb_log_toggled (GtkToggleButton*, gpointer);

void start_athread (void);
gboolean stop_athread (gpointer);
void on_toolb_connect_clicked (GtkToolButton*, gpointer);
void on_toolb_dconnect_clicked (GtkToolButton*, gpointer);
void on_toolb_sitem_clicked (GtkToolButton*, gpointer);
void on_noteb_client_switch_page (GtkNotebook*, GtkNotebookPage*, gint, gpointer);

void on_button_cchat_clicked (GtkWidget*, gpointer);
void on_button_clogout_clicked (GtkWidget*, gpointer);
void on_button_crestart_clicked (GtkWidget*, gpointer);
void on_button_cshutdown_clicked (GtkWidget*, gpointer);
void on_button_cmove_clicked (GtkWidget*, gpointer);
void on_button_cedit_clicked (GtkWidget*, gpointer);
void on_button_cenable_clicked (GtkWidget*, gpointer);
void on_button_cdisable_clicked (GtkWidget*, gpointer);

void on_button_lfilter_clicked (GtkWidget*, gpointer);
void on_button_ldetail_clicked (GtkWidget*, gpointer);
void on_button_ltotal_clicked (GtkWidget*, gpointer);
void on_button_ldel_clicked (GtkWidget*, gpointer);
void on_button_lexport_clicked (GtkWidget*, gpointer);
void on_button_lrefresh_clicked (GtkWidget*, gpointer);

/* window_setting_server */
gboolean on_window_setting_server_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_checkb_logo_window_setting_server_toggled (GtkToggleButton*, gpointer);
void on_render_play_window_setting_server_toggled (GtkCellRendererToggle*, gchar*, gpointer);
void on_treeview_sound_window_setting_server_changed (GtkTreeSelection*, gpointer);
void on_button_browse_window_setting_server_clicked (GtkWidget*, gpointer);
void on_button_preview_window_setting_server_clicked (GtkWidget*, gpointer);
void on_button_ok_window_setting_server_clicked (GtkWidget*, gpointer);
void on_button_logo_window_setting_server_clicked (GtkWidget*, gpointer);
void on_entry_suser_window_setting_server_changed (GtkEditable*, gpointer);
void on_entry_spasswd_window_setting_server_changed (GtkEditable*, gpointer);
void on_entry_spasswdc_window_setting_server_changed (GtkEditable*, gpointer);
void on_button_sset_window_setting_server_clicked (GtkWidget*, gpointer);
void on_entry_cuser_window_setting_server_changed (GtkEditable*, gpointer);
void on_entry_cpasswd_window_setting_server_changed (GtkEditable*, gpointer);
void on_entry_cpasswdc_window_setting_server_changed (GtkEditable*, gpointer);
void on_button_cset_window_setting_server_clicked (GtkWidget*, gpointer);
gboolean on_textv_addr_window_setting_server_key_press_event (GtkWidget*, GdkEventKey*, gpointer);

/* window_setting_client */
gboolean on_window_setting_client_delete_event (GtkWidget*, GdkEvent*, gpointer);

/* window_setting_log */
void on_combob_show_window_setting_log_changed (GtkComboBox*, gpointer);
void on_combob_del_window_setting_log_changed (GtkComboBox*, gpointer);
void on_checkb_del_window_setting_log_toggled (GtkToggleButton*, gpointer);
void on_entry_user_window_setting_log_changed (GtkEditable*, gpointer);
void on_entry_pass_window_setting_log_changed (GtkEditable*, gpointer);
void on_entry_passc_window_setting_log_changed (GtkEditable*, gpointer);
void on_button_set_window_setting_log_clicked (GtkButton*, gpointer);
void on_window_setting_log_entry_name_changed (GtkEditable*, gpointer);
void on_window_setting_log_entry_passwd_changed (GtkEditable*, gpointer);
void on_window_setting_log_entry_passwdc_changed (GtkEditable*, gpointer);
void on_window_setting_log_button_save_clicked (GtkButton*, gpointer);
void on_button_ok_window_setting_log_clicked (GtkButton*, gpointer);

/* window_setting_client */
void on_button_add_window_setting_client_clicked (GtkButton*, gpointer);
void on_button_edit_window_setting_client_clicked (GtkButton*, gpointer);
void on_button_del_window_setting_client_clicked (GtkButton*, gpointer);

/* window_update_client */
void on_window_update_client_entry_ip_insert_text (GtkEditable*, gchar*, gint, gint*, gpointer);
gboolean on_window_update_client_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_ok_window_update_client_clicked (GtkButton*, gpointer);

/* window_scost */
gboolean on_window_setting_cost_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_scost_add_clicked (GtkButton*, gpointer);
void on_button_scost_edit_clicked (GtkButton*, gpointer);
void on_button_scost_del_clicked (GtkButton*, gpointer);
void on_button_default_window_setting_cost_clicked (GtkButton*, gpointer);
void on_button_add_rotation_window_setting_cost_clicked (GtkWidget*, gpointer);
void on_button_edit_rotation_window_setting_cost_clicked (GtkWidget*, gpointer);
void on_button_del_rotation_window_setting_cost_clicked (GtkWidget*, gpointer);

/* window_update_cost */
gboolean on_window_update_cost_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_radiob_window_cost_mode1_toggled (GtkToggleButton*, gpointer);
void on_radiob_window_cost_mode2_toggled (GtkToggleButton*, gpointer);
void set_label_sumr_window_update_cost (void);
void on_button_ok_window_update_cost_clicked (GtkButton*, gpointer);

/* window_rotate_cost */
gboolean on_window_rotate_cost_deleve_event (GtkWidget*, GdkEvent*, gpointer);
void on_checkb_active_window_rotate_cost_toggled (GtkToggleButton*, gpointer);
void on_button_ok_window_rotate_cost_clicked (GtkWidget*, gpointer);

/* window_setting_prepaid */
gboolean on_window_setting_prepaid_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_add_window_setting_prepaid_clicked (GtkButton*, gpointer);
void on_button_edit_window_setting_prepaid_clicked (GtkButton*, gpointer);
void on_button_del_window_setting_prepaid_clicked (GtkButton*, gpointer);

/* window_update_prepaid */
void on_button_ok_window_update_prepaid_clicked (GtkButton*, gpointer);
gboolean on_window_update_prepaid_delete_event (GtkWidget*, GdkEvent*, gpointer);

/* window_setting_item */
gboolean on_window_setting_item_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_add_window_setting_item_clicked (GtkButton*, gpointer);
void on_button_edit_window_setting_item_clicked (GtkButton*, gpointer);
void on_button_del_window_setting_item_clicked (GtkButton*, gpointer);

/* window_update_item */
gboolean on_window_update_item_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_ok_window_update_item_clicked (GtkButton*, gpointer);

/* window_smember */
void on_treev_window_member_column_clicked (GtkTreeViewColumn*, gpointer);
void on_button_add_window_group_member_clicked (GtkButton*, gpointer);
void on_button_edit_window_group_member_clicked (GtkButton*, gpointer);
void on_button_delete_window_group_member_clicked (GtkButton*, gpointer);

/* window_update_group_member*/
void on_button_ok_window_update_group_member_clicked (GtkButton*, gpointer);

/* window_member */
void on_button_window_member_add_clicked (GtkButton*, gpointer);
void on_button_window_member_edit_clicked (GtkButton*, gpointer);
void on_button_window_member_del_clicked (GtkButton*, gpointer);
void on_button_window_member_search_clicked (GtkButton*, gpointer);

/* window_umember */
void on_window_umember_destroy (GtkWidget*, gpointer);
void on_entry_window_umember_pass_changed (GtkEntry*, gpointer);
void on_button_window_umember_ok_clicked (GtkButton*, gpointer);

/* window_member_search */
gboolean on_window_member_search_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_checkb_window_member_search_toggled (GtkToggleButton*, gpointer);
void on_checkb_window_member_search_register1_toggled (GtkToggleButton*, gpointer);
void on_toggleb_window_member_search_register1_toggled (GtkToggleButton*, gpointer);
void on_checkb_window_member_search_status_toggled (GtkToggleButton*, gpointer);
void on_button_window_member_search_ok_clicked (GtkButton*, gpointer);

/* window_calendar_member */
gboolean on_window_calendar_member_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_calendar_window_calendar_member_day_selected_double_click (GtkCalendar*, gpointer);

/* wchat->window */
gboolean on_wchat_window_key_press_event (GtkWidget*, GdkEventKey*, ClientData*);
void on_wchat_bsend_clicked (GtkWidget*, ClientData*);

/* window_logf */
gboolean on_window_logf_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_wlogfok_clicked (GtkButton*, gpointer);
void on_checkb_wlogfdate_toggled (GtkToggleButton*, gpointer);
void on_toggleb_wlogfdate_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogfdate2_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogfclient_toggled (GtkToggleButton*, gpointer);
void on_checkb_username_window_search_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogftype_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogfstart_toggled (GtkToggleButton*, gpointer);
void on_toggleb_wlogfstart_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogfend_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogfcost_toggled (GtkToggleButton*, gpointer);
void on_checkb_wlogfcost2_toggled (GtkToggleButton*, gpointer);
gboolean on_window_detail_delete_event (GtkWidget*, GdkEvent*, gpointer);

/* window_logtotal */
gboolean on_window_logtotal_delete_event (GtkWidget*, GdkEvent*, gpointer);

/* window_move */
void on_spinb_editacost_value_changed (GtkSpinButton *, gpointer);
gboolean on_window_move_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_moveok_clicked (GtkWidget*, gpointer);

/* window_edit */
void on_window_edit_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_editok_clicked (GtkWidget*, gpointer);

/* window_calendar */
gboolean on_window_calendar_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_calendar_day_selected_double_click (GtkCalendar*, gpointer);

/* window_time */
gboolean on_window_time_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_button_wtimeok_clicked (GtkButton*, gpointer);

#endif /* __CALLBACK_H__ */

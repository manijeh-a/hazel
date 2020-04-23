module Js = Js_of_ocaml.Js;
module Dom = Js_of_ocaml.Dom;
module Dom_html = Js_of_ocaml.Dom_html;
open Incr_dom;

// https://github.com/janestreet/incr_dom/blob/6aa4aca2cfc82a17bbcc0424ff6b0ae3d6d8d540/example/text_input/README.md
// https://github.com/janestreet/incr_dom/blob/master/src/app_intf.ml

module Model = Model;
module Action = Update.Action;
module State = State;

let on_startup = (~schedule_action, _) => {
  let _ =
    JSUtil.listen_to_t(
      Dom.Event.make("selectionchange"),
      Dom_html.document,
      _ => {
        let anchorNode = Dom_html.window##getSelection##.anchorNode;
        let contenteditable = JSUtil.force_get_elem_by_id("contenteditable");
        if (JSUtil.div_contains_node(contenteditable, anchorNode)) {
          schedule_action(Update.Action.SelectionChange);
        };
      },
    );
  Dom_html.window##.onfocus :=
    Dom_html.handler(_ => {
      schedule_action(Update.Action.FocusWindow);
      Js._true;
    });
  schedule_action(Update.Action.FocusCell);
  Async_kernel.Deferred.return(
    State.{setting_caret: ref(false), changing_cards: ref(false)},
  );
};

let restart_caret_animation = () => {
  let caret = JSUtil.force_get_elem_by_id("caret");
  caret##.classList##remove(Js.string("blink"));
  caret##focus;
  caret##.classList##add(Js.string("blink"));
};

let scroll_history_panel_entry = entry_elem => {
  let panel_rect =
    JSUtil.force_get_elem_by_id("history-body")##getBoundingClientRect;
  let entry_rect = entry_elem##getBoundingClientRect;
  if (entry_rect##.top < panel_rect##.top) {
    entry_elem##scrollIntoView(Js._true);
  } else if (entry_rect##.bottom > panel_rect##.bottom) {
    entry_elem##scrollIntoView(Js._false);
  };
};

let create =
    (
      model: Incr.t(Model.t),
      ~old_model as _: Incr.t(Model.t),
      ~inject: Update.Action.t => Vdom.Event.t,
    ) => {
  open Incr.Let_syntax;
  let%map model = model;
  Component.create(
    ~apply_action=Update.apply_action(model),
    ~on_display=
      (state: State.t, ~schedule_action as _: Update.Action.t => unit) => {
        let undo_history = model |> Model.get_undo_history;
        if (!UndoHistory.is_empty(undo_history)) {
          let entry_elem = JSUtil.force_get_elem_by_id("cur-selected-entry");
          scroll_history_panel_entry(entry_elem);
        };

        let path = model |> Model.get_program |> Program.get_path;
        if (state.changing_cards^) {
          state.changing_cards := false;
          let (anchor_node, anchor_offset) =
            path |> UHCode.caret_position_of_path;
          state.setting_caret := true;
          JSUtil.set_caret(anchor_node, anchor_offset);
        } else if (model.is_cell_focused) {
          let (expected_node, expected_offset) =
            path |> UHCode.caret_position_of_path;
          let (actual_node, actual_offset) = JSUtil.get_selection_anchor();
          if (actual_node === expected_node
              && actual_offset === expected_offset) {
            state.setting_caret := false;
          } else {
            state.setting_caret := true;
            JSUtil.set_caret(expected_node, expected_offset);
          };
          restart_caret_animation();
        };
      },
    model,
    Page.view(~inject, model),
  );
};

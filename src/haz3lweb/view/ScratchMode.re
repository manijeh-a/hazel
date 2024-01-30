open Haz3lcore;

type state = (Id.t, Editor.t);

let view =
    (
      ~inject,
      ~ui_state: Model.ui_state,
      ~settings: Settings.t,
      ~color_highlighting,
      ~results: ModelResults.t,
      ~result_key,
      ~statics as {editor, error_ids, _}: Editor.statics,
    ) => {
  //TODO(andrew): cleanup footer
  let result = ModelResults.get(results, result_key);
  let footer =
    settings.core.statics
      ? result
        |> Option.map(result =>
             Cell.footer(~settings, ~inject, ~ui_state, ~result, ~result_key)
           )
        |> Option.to_list
        |> List.flatten
      : [];
  [
    Cell.editor_view(
      ~inject,
      ~ui_state,
      ~settings,
      ~target_id="code-container",
      ~error_ids,
      ~test_results=result |> Util.OptUtil.and_then(ModelResult.test_results),
      ~footer,
      ~color_highlighting,
      Editor.get_syntax(editor),
    ),
  ];
};

let export_button = state =>
  Widgets.button_named(
    Icons.star,
    _ => {
      let json_data = ScratchSlide.export(state);
      JsUtil.download_json("hazel-scratchpad", json_data);
      Virtual_dom.Vdom.Effect.Ignore;
    },
    ~tooltip="Export Scratchpad",
  );
let import_button = inject =>
  Widgets.file_select_button_named(
    "import-scratchpad",
    Icons.star,
    file => {
      switch (file) {
      | None => Virtual_dom.Vdom.Effect.Ignore
      | Some(file) => inject(UpdateAction.InitImportScratchpad(file))
      }
    },
    ~tooltip="Import Scratchpad",
  );

let reset_button = inject =>
  Widgets.button_named(
    Icons.trash,
    _ => {
      let confirmed =
        JsUtil.confirm(
          "Are you SURE you want to reset this scratchpad? You will lose any existing code.",
        );
      if (confirmed) {
        inject(UpdateAction.ResetCurrentEditor);
      } else {
        Virtual_dom.Vdom.Effect.Ignore;
      };
    },
    ~tooltip="Reset Scratchpad",
  );

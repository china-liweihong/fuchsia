// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    crate::{
        config::{self, OutputFormat, ProgramStateHolder},
        Options,
    },
    anyhow::{Context as _, Error},
    triage::{analyze, ActionResults, ActionResultFormatter},
};

/// The entry point for the CLI app.
pub struct App {
    options: Options,
}

impl App {
    /// Creates a new App with the given options.
    pub fn new(options: Options) -> App {
        App { options }
    }

    /// Runs the App.
    ///
    /// This method consumes self and returns a [RunResult] which can be used
    /// to examine results. If an error occurs during the running of the app
    /// it will be returned as an Error.
    pub fn run(self) -> Result<RunResult, Error> {
        // TODO(fxb/50449): Use 'argh' crate.
        let ProgramStateHolder { parse_result, diagnostic_data, output_format } =
            config::initialize(self.options)?;

        let action_results = analyze(&diagnostic_data, &parse_result)?;

        Ok(RunResult::new(output_format, action_results))
    }
}

/// The result of calling App::run.
pub struct RunResult {
    output_format: OutputFormat,
    action_results: Vec<ActionResults>,
}

impl RunResult {
    /// Creates a new RunResult struct. This method is intended to be used by the
    /// App:run method.
    fn new(output_format: OutputFormat, action_results: Vec<ActionResults>) -> RunResult {
        RunResult { output_format, action_results }
    }

    /// Writes the contents of the run to the provided writer.
    ///
    /// This method can be used to output the results to a file or stdout.
    pub fn write_report(&self, dest: &mut dyn std::io::Write) -> Result<(), Error> {
        let results_formatter = ActionResultFormatter::new(self.action_results.iter().collect());
        let output = match self.output_format {
            OutputFormat::Text => results_formatter.to_warnings(),
        };
        dest.write_fmt(format_args!("{}\n", output)).context("failed to write to destination")?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use {super::*, triage::ActionResults};

    macro_rules! make_action_result {
        ($source:expr, $($action:expr => $r:literal),+) => {
            {
                let mut result = ActionResults::new($source);
                $(
                    result.set_result($action, $r);
                )*
                result
            }
        };
    }

    #[test]
    fn test_output_text_no_warnings() -> Result<(), Error> {
        let action_results = vec![make_action_result!("foo", "a" => true)];
        let run_result = RunResult::new(OutputFormat::Text, action_results);

        let mut dest = vec![];
        run_result.write_report(&mut dest)?;

        let output = String::from_utf8(dest)?;
        assert_eq!("Warnings for foo\n\n", output);

        Ok(())
    }

    #[test]
    fn test_output_text_with_warnings() -> Result<(), Error> {
        let mut action_result = make_action_result!("foo", "a" => true);
        action_result.add_warning("fail".to_string());

        let action_results = vec![action_result];
        let run_result = RunResult::new(OutputFormat::Text, action_results);

        let mut dest = vec![];
        run_result.write_report(&mut dest)?;

        let output = String::from_utf8(dest)?;
        assert_eq!("Warnings for foo\nfail\n\n", output);

        Ok(())
    }
}

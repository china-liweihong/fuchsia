// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    crate::config::args::ConfigCommand, argh::FromArgs,
    ffx_run_component::args::RunComponentCommand, ffx_test::args::TestCommand,
};

#[derive(FromArgs, Debug, PartialEq)]
/// Fuchsia Development Bridge
pub struct Ffx {
    #[argh(option)]
    /// configuration information
    pub config: Option<String>,

    #[argh(subcommand)]
    pub subcommand: Subcommand,
}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand, name = "daemon", description = "run as daemon")]
pub struct DaemonCommand {}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand, name = "echo", description = "run echo test")]
pub struct EchoCommand {
    #[argh(positional)]
    /// text string to echo back and forth
    pub text: Option<String>,
}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand, name = "list", description = "list connected devices")]
pub struct ListCommand {
    #[argh(positional)]
    pub nodename: Option<String>,
}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand, name = "quit", description = "kills a running daemon")]
pub struct QuitCommand {}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand)]
pub enum Subcommand {
    Daemon(DaemonCommand),
    Echo(EchoCommand),
    List(ListCommand),
    RunComponent(RunComponentCommand),
    Config(ConfigCommand),
    Test(TestCommand),
    Quit(QuitCommand),
}

#[cfg(test)]
mod tests {
    use super::*;
    const CMD_NAME: &'static [&'static str] = &["ffx"];

    #[test]
    fn test_echo() {
        fn check(args: &[&str], expected_echo: &str) {
            assert_eq!(
                Ffx::from_args(CMD_NAME, args),
                Ok(Ffx {
                    config: None,
                    subcommand: Subcommand::Echo(EchoCommand {
                        text: Some(expected_echo.to_string()),
                    })
                })
            )
        }

        let echo = "test-echo";

        check(&["echo", echo], echo);
    }

    #[test]
    fn test_daemon() {
        fn check(args: &[&str]) {
            assert_eq!(
                Ffx::from_args(CMD_NAME, args),
                Ok(Ffx { config: None, subcommand: Subcommand::Daemon(DaemonCommand {}) })
            )
        }

        check(&["daemon"]);
    }

    #[test]
    fn test_list() {
        fn check(args: &[&str], nodename: String) {
            assert_eq!(
                Ffx::from_args(CMD_NAME, args),
                Ok(Ffx {
                    config: None,
                    subcommand: Subcommand::List(ListCommand { nodename: Some(nodename) })
                })
            )
        }

        let nodename = String::from("thumb-set-human-neon");
        check(&["list", &nodename], nodename.clone());
    }
}

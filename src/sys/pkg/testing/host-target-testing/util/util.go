// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package util

import (
	"archive/tar"
	"bufio"
	"bytes"
	"compress/gzip"
	"context"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"go.fuchsia.dev/fuchsia/tools/lib/logger"
)

// RunCommand executes a command on the host and returns the stdout and stderr
// as byte strings.
func RunCommand(ctx context.Context, name string, arg ...string) ([]byte, []byte, error) {
	logger.Infof(ctx, "running: %s %q", name, arg)
	c := exec.CommandContext(ctx, name, arg...)
	var o bytes.Buffer
	var e bytes.Buffer
	c.Stdout = &o
	c.Stderr = &e
	err := c.Run()
	stdout := o.Bytes()
	stderr := e.Bytes()
	return stdout, stderr, err
}

// Untar untars a tar.gz file into a directory.
func Untar(ctx context.Context, dst string, src string) error {
	logger.Infof(ctx, "untarring %s into %s", src, dst)

	f, err := os.Open(src)
	if err != nil {
		return err
	}
	defer f.Close()

	gz, err := gzip.NewReader(f)
	if err != nil {
		return err
	}
	defer gz.Close()

	tr := tar.NewReader(gz)

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		default:
			header, err := tr.Next()
			if err == io.EOF {
				return nil
			} else if err != nil {
				return err
			}

			if err := untarNext(dst, tr, header); err != nil {
				return err
			}
		}
	}
}

func untarNext(dst string, tr *tar.Reader, header *tar.Header) error {
	path := filepath.Join(dst, header.Name)
	info := header.FileInfo()
	if info.IsDir() {
		if err := os.MkdirAll(path, info.Mode()); err != nil {
			return err
		}
	} else {
		if err := os.MkdirAll(filepath.Dir(path), 0755); err != nil {
			return err
		}

		// Skip entry if path already exists.
		if _, err := os.Stat(path); err == nil {
			return nil
		}

		err := AtomicallyWriteFile(path, info.Mode(), func(f *os.File) error {
			_, err := io.Copy(f, tr)
			return err
		})
		if err != nil {
			return err
		}
	}

	return nil
}

func ParsePackageList(rd io.Reader) (map[string]string, error) {
	scanner := bufio.NewScanner(rd)
	packages := make(map[string]string)
	for scanner.Scan() {
		s := strings.TrimSpace(scanner.Text())
		entry := strings.Split(s, "=")
		if len(entry) != 2 {
			return nil, fmt.Errorf("parser: entry format: %q", s)
		}
		packages[entry[0]] = entry[1]
	}
	if err := scanner.Err(); err != nil {
		return nil, err
	}

	return packages, nil
}

func AtomicallyWriteFile(path string, mode os.FileMode, writeFileFunc func(*os.File) error) error {
	dir := filepath.Dir(path)
	basename := filepath.Base(path)

	tmpfile, err := ioutil.TempFile(dir, basename)
	if err != nil {
		return err
	}
	defer func() {
		if tmpfile != nil {
			tmpfile.Close()
			os.Remove(tmpfile.Name())
		}
	}()

	if err = writeFileFunc(tmpfile); err != nil {
		return err
	}

	if err = os.Chmod(tmpfile.Name(), mode); err != nil {
		return err
	}

	// Now that we've written the file, do an atomic swap of the filename into place.
	if err := os.Rename(tmpfile.Name(), path); err != nil {
		return err
	}
	if err := tmpfile.Close(); err != nil {
		return err
	}
	tmpfile = nil

	return nil
}

// RunWithTimeout runs a closure to completion, or returns an error if it times
// out.
func RunWithTimeout(ctx context.Context, timeout time.Duration, f func() error) error {
	return RunWithDeadline(ctx, time.Now().Add(timeout), f)
}

// RunWithDeadline runs a closure to runs the closure in a goroutine
func RunWithDeadline(ctx context.Context, deadline time.Time, f func() error) error {
	ctx, cancel := context.WithDeadline(ctx, deadline)
	defer cancel()

	ch := make(chan error, 1)
	go func() {
		ch <- f()
	}()

	select {
	case err := <-ch:
		return err
	case <-ctx.Done():
		return fmt.Errorf("Function timed out: %w", ctx.Err())
	}
}

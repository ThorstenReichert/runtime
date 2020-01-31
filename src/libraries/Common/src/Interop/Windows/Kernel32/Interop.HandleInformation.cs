// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;

internal static partial class Interop
{
    internal static partial class Kernel32
    {
        [Flags]
        internal enum HandleFlags: uint
        {
            None = 0,
            HANDLE_FLAG_INHERIT = 1,
            HANDLE_FLAG_PROTECT_FROM_CLOSE = 2
        }

        [DllImport(Libraries.Kernel32, SetLastError = true)]
        internal static extern bool SetHandleInformation(SafeHandle handle, HandleFlags mask, HandleFlags flags);
    }
}

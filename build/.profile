# ~/.profile: executed by the command interpreter for login shells.
# This file is not read by bash(1), if ~/.bash_profile or ~/.bash_login
# exists.
# see /usr/share/doc/bash/examples/startup-files for examples.
# the files are located in the bash-doc package.

# the default umask is set in /etc/profile; for setting the umask
# for ssh logins, install and configure the libpam-umask package.
#umask 022

# if running bash
if [ -n "$BASH_VERSION" ]; then
    # include .bashrc if it exists
    if [ -f "$HOME/.bashrc" ]; then
	. "$HOME/.bashrc"
    fi
fi

# set PATH so it includes user's private bin if it exists
if [ -d "$HOME/bin" ] ; then
    PATH="$HOME/bin:$PATH"
fi

# set PATH so it includes user's private bin if it exists
if [ -d "$HOME/.local/bin" ] ; then
    PATH="$HOME/.local/bin:$PATH"
fi

# set PATH so it includes user's private scripts if it exists
if [ -d "$HOME/.local/scripts" ] ; then
    PATH="$HOME/.local/scripts:$PATH"
fi

alias fgs='git status '
alias fga='git add '
alias fgl='git log'
alias fap='ax_push.sh'
alias fgp='git pull'
alias fgc='git commit '
alias fgh='git reset --hard HEAD^'
alias fgca='git commit --amend'
alias fca='cd ~/repos'
alias fcb='cd ~/repos/build'
alias fcp='cd ~/repos/app/IPCDemo'
alias fct='cd ~/repos/app/IPCTest'
alias fco='cd ~/repos/app/opal'
alias fbd='make p=AX620_demo'
alias fbn='make p=AX620U_nand'

local function save_and_run()
  vim.cmd([[wa]])
  vim.cmd([[belowright split]])
  vim.cmd([[resize -4]])
  vim.cmd([[terminal make && bin/codewars]])
end

local function save_and_debug()
  vim.cmd([[wa]])
  vim.cmd([[terminal make && gdb bin/codewars]])
end

local opts = { noremap = true, silent = true }
vim.keymap.set("n", "<C-R>", save_and_run, opts)
vim.keymap.set("n", "<F5>", save_and_debug, opts)
vim.keymap.set("i", "<A-/>", "; ----------------------------------------")

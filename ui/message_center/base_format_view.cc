// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/base_format_view.h"

#include "base/i18n/time_formatting.h"
#include "grit/ui_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"

namespace message_center {

const SkColor kNotificationColor = SkColorSetRGB(0xfe, 0xfe, 0xfe);
const SkColor kNotificationReadColor = SkColorSetRGB(0xfa, 0xfa, 0xfa);

const int kBaseFormatPrimaryIconWidth = 64;
const int kBaseFormatSecondaryIconWidth = 16;
const int kBaseFormatTimestampWidth = 60;
const int kBaseFormatButtonWidth = 60;
const int kBaseFormatPaddingBetweenItems = 10;
const int kBaseFormatOuterHorizontalPadding = 18;

BaseFormatView::BaseFormatView(
    NotificationList::Delegate* list_delegate,
    const NotificationList::Notification& notification)
    : MessageView(list_delegate, notification),
      button_one_(NULL),
      button_two_(NULL) {
}

BaseFormatView::~BaseFormatView() {
}

void BaseFormatView::SetUpView() {
  SkColor bg_color = notification().is_read ?
      kNotificationReadColor : kNotificationColor;
  set_background(views::Background::CreateSolidBackground(bg_color));

  views::ImageView* icon = new views::ImageView;
  icon->SetImageSize(
      gfx::Size(kBaseFormatPrimaryIconWidth, kBaseFormatPrimaryIconWidth));
  icon->SetImage(notification().primary_icon);

  views::ImageView* second_icon = new views::ImageView;
  second_icon->SetImageSize(
      gfx::Size(kBaseFormatSecondaryIconWidth, kBaseFormatSecondaryIconWidth));
  // TODO: set up second image
  second_icon->SetImage(notification().primary_icon);

  views::Label* title = new views::Label(notification().title);
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetFont(title->font().DeriveFont(0, gfx::Font::BOLD));

  views::Label* message = new views::Label(notification().message);
  message->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  message->SetMultiLine(true);
  message->SetElideBehavior(views::Label::ELIDE_AT_END);
  message->SizeToFit(
      kBaseFormatButtonWidth * 2 + kBaseFormatPaddingBetweenItems * 3 +
      kBaseFormatTimestampWidth);

  views::Label* timestamp = NULL;
  if (notification().timestamp != base::Time()) {
    timestamp = new views::Label(
        base::TimeFormatTimeOfDay(notification().timestamp));
    timestamp->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  }

  // TODO(miket): unreadCount

  if (notification().button_titles.size() > 0) {
    button_one_ = new views::LabelButton(this, notification().button_titles[0]);
    button_one_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    button_one_->SetNativeTheme(true);
  }
  if (notification().button_titles.size() > 1) {
    button_two_ = new views::LabelButton(this, notification().button_titles[1]);
    button_two_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    button_two_->SetNativeTheme(true);
  }

  views::Label* expanded_message = new views::Label(
      notification().expanded_message);
  expanded_message->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  expanded_message->SetMultiLine(true);
  expanded_message->SizeToFit(
      kBaseFormatButtonWidth * 2 + kBaseFormatPaddingBetweenItems);

  // TODO(miket): Image thumbnail for image-type notifications (imageUrl)

  views::GridLayout* layout = new views::GridLayout(this);
  SetLayoutManager(layout);

  views::ColumnSet* columns = layout->AddColumnSet(0);

  const int padding_width = kBaseFormatOuterHorizontalPadding / 2;
  columns->AddPaddingColumn(0, padding_width);

  // Column 0: Notification Icon.
  columns->AddColumn(views::GridLayout::CENTER, views::GridLayout::LEADING,
                     0, /* resize percent */
                     views::GridLayout::FIXED,
                     kBaseFormatPrimaryIconWidth, kBaseFormatPrimaryIconWidth);
  columns->AddPaddingColumn(0, kBaseFormatPaddingBetweenItems);

  // Column 1: Notification message text and first button.
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                     100, /* resize percent */
                     views::GridLayout::USE_PREF,
                     kBaseFormatButtonWidth, kBaseFormatButtonWidth);
  columns->AddPaddingColumn(0, kBaseFormatPaddingBetweenItems);

  // Column 2: Notification message text and second button.
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                     100, /* resize percent */
                     views::GridLayout::USE_PREF,
                     kBaseFormatButtonWidth, kBaseFormatButtonWidth);
  columns->AddPaddingColumn(0, kBaseFormatPaddingBetweenItems);

  // Column 3: Notification message text and timestamp.
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                     0, /* resize percent */
                     views::GridLayout::FIXED,
                     kBaseFormatTimestampWidth, kBaseFormatTimestampWidth);
  columns->AddPaddingColumn(0, kBaseFormatPaddingBetweenItems);

  // Column 4: Close button and secondary icon.
  columns->AddColumn(views::GridLayout::CENTER, views::GridLayout::BASELINE,
                     0, /* resize percent */
                     views::GridLayout::FIXED,
                     kBaseFormatSecondaryIconWidth,
                     kBaseFormatSecondaryIconWidth);
  columns->AddPaddingColumn(0, kBaseFormatPaddingBetweenItems);

  // Lay out rows.
  // Row 0: Just timestamp and close box.
  layout->StartRow(0, 0);
  layout->SkipColumns(5);
  if (timestamp)
    layout->AddView(timestamp, 1, 1);
  else
    layout->SkipColumns(1);
  layout->AddView(close_button(), 1, 1);

  // Row 1: Big icon, title.
  layout->StartRow(0, 0);
  layout->AddView(icon, 1, 3);
  layout->AddView(title, 3, 1);
  layout->SkipColumns(1);

  // Row 2: Continuation of big icon, message.
  layout->StartRow(0, 0);
  layout->SkipColumns(1);
  layout->AddView(message, 3, 1);
  layout->SkipColumns(1);
  layout->AddPaddingRow(0, kBaseFormatPaddingBetweenItems);

  // Row 3: Continuation of big icon, two buttons, secondary icon.
  layout->StartRow(0,0);
  layout->SkipColumns(1);
  if (button_two_) {
    layout->AddView(button_one_, 1, 1);
    layout->AddView(button_two_, 1, 1);
  } else if (button_one_) {
    layout->AddView(button_one_, 1, 1);
    layout->SkipColumns(1);
  } else {
    layout->SkipColumns(3);  // two buttons plus padding
  }
  layout->SkipColumns(1);
  layout->AddView(second_icon, 1, 1);
  layout->AddPaddingRow(0, kBaseFormatPaddingBetweenItems);

  // Row 4: Secondary message.
  layout->StartRow(0,0);
  layout->SkipColumns(1);
  layout->AddView(expanded_message, 3, 1);

  // A final bit of padding to make it look nice.
  layout->AddPaddingRow(0, kBaseFormatPaddingBetweenItems);
}

void BaseFormatView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  // TODO(miket): consider changing the _one, _two etc. widgets to an array or
  // map so that we can move this behavior to the superclass. It seems like
  // something we wouldn't want to keep recoding for each subclass.
  if (sender == button_one_) {
    list_delegate()->OnButtonClicked(notification().id, 0);
  } else if (sender == button_two_) {
    list_delegate()->OnButtonClicked(notification().id, 1);
  } else {
    MessageView::ButtonPressed(sender, event);
  }
}

}  // namespace message_center
